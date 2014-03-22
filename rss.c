#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sqlite3.h>

#include "mrss.h"
#include "Gigabit.h"
#include "libircclient.h"
#include "rss.h"
#include "delayed_send.h"
#define WAIT_TIME 1800

sqlite3 *h;


void *
run_rss (void *arg)
{
  irc_session_t *session = (irc_session_t *) arg;
  context_t *context = (context_t *) irc_get_ctx (session);

  if (sqlite3_open (context->rss_db, &h))
    {
      printf ("Error opening rss db\n");
      fflush (stdout);
      // exit (1);
      return;
    }
  else
    sqlite3_exec (h,
		  "CREATE TABLE IF NOT EXISTS rss (title TEXT PRIMARY KEY,link TEXT NOT NULL,posted INTEGER)",
		  0, 0, 0);
  for (;;)
    {
      //  printf("Fetching...\n\r");
      fflush (stdout);
      sleep (WAIT_TIME);

      if (rss_fetch (arg) == 0xDEAD)
	printf ("RSS Fetch is 0xDEAD!!\a\n");
    }
}

int
compare (char *a, char *b)
{
  int i = 0, sz_a, sz_b;
  sz_a = strlen (a);
  sz_b = strlen (b);

  if (sz_a != sz_b)
    return 1;
  for (i; i < sz_a; i++)
    {
      if (a[i] != b[i])
	return 1;
    }
  return 0;
}

//char *
// md5_enc (char *str, char *enc)
// {
// 
//   md5_state_t state;
//   md5_byte_t digest[1];
//   md5_init (&state);
//   int i;
//   md5_append (&state, (const md5_byte_t *) str, strlen (str));
//   md5_finish (&state, digest);
//   for (i = 0; i < 16; ++i)
//     sprintf (enc + i * 2, "%02x", digest[i]);
//   return enc;
// }

int
rss_fetch (void *arg)
{
  irc_session_t *session = (irc_session_t *) arg;
  context_t *context = (context_t *) irc_get_ctx (session);
  mrss_t *data;
  mrss_error_t ret;
  mrss_item_t *item;
  mrss_tag_t *tag;
  CURLcode code;
  sqlite3_stmt *statement;

  FILE *flist = fopen ("./rss.list", "r"),
    *subscribers = fopen (context->subscribers, "r");
  char feed_list[64][256], subscriber_list[1024][256], post[1024],
    db_query[1024], line[1024];
  int i = 0, j = 0, k = 0, o = 0, r = 0, sent = 0, total = 0;
  if (flist == NULL || subscribers == NULL)
    return 0xDEAD;
  for (i = 0; i < 64 && !feof (flist); i++)
    {
      fgets (feed_list[i], 256, flist);
    }
  fclose (flist);
  for (o = 0; o < 1024 && !feof (subscribers); o++)
    {
      fgets (subscriber_list[o], 256, subscribers);

      trim (subscriber_list[o]);
      if (strlen (subscriber_list[o]) < 3)
	--o;
    }
  fclose (subscribers);
  for (j = 0; j < (i - 1); j++)
    {
      ret =
	mrss_parse_url_with_options_and_error (feed_list[j], &data, NULL,
					       &code);


      if (ret)
	{
	  //fprintf (stdout, "MRSS return error: %s error\n",feed_list[j-1]);
	  //ret ==
	  //MRSS_ERR_DOWNLOAD ? mrss_curl_strerror (code) :
	  // mrss_strerror (ret));
	  break;
	}
      for (item = data->item; item; item = item->next)
	{
	  if (strlen (item->title) > 3)
	    {
	      sanitize (item->title);
	      snprintf (db_query, 1024, "SELECT * FROM rss WHERE title='%s'",
			item->title);
//                     printf("$$%s$$\n",db_query);

	      if (sqlite3_prepare_v2 (h, db_query, -1, &statement, 0) ==
		  SQLITE_OK)
		{
		  r = sqlite3_step (statement);

		  if (statement != NULL && r == SQLITE_DONE)
		    {

		      snprintf (db_query, 1024,
				"INSERT INTO rss VALUES('%s','%s',1)",
				item->title, item->link);

		      sqlite3_exec (h, db_query, 0, 0, 0);
		      snprintf (post, 1024, "%s [%s]", item->title,
				item->link);
		      fflush (stdout);
		      ++total;
		      if (total < 12)
			{
			  for (k = 0; k < o; k++)
			    {

			      push_message (subscriber_list[k], post);

			      //   printf ("~~> <%s>%s\n", post);



			    }
			}
		      else
			{
			  for (k = 0; k < o; k++)
			    {
			      snprintf (post, 1024,
					"More feeds will be available later,please wait %d minutes for next update",
					WAIT_TIME / 60);
			      push_message (subscriber_list[k], post);
			      return;
			    }
			}


		    }

		}
	      else
		{
		  printf ("prepare error!!\n");
		}
	    }

	}

    }
  mrss_free (data);
//   sqlite3_close (&h);
  return 0;

}
