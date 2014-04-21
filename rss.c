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

  int i;
  for (;;)
    {

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
      for (i = 0; i < 7; i++)
	{
	  fflush (stdout);
	  sleep (WAIT_TIME);

	  if (rss_fetch (arg) == 0xDEAD)
	    printf ("RSS Fetch is 0xDEAD!!\a\n");
	}
      sqlite3_close (h);
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

  char post[1024], db_query[1024], line[1024];
  int i, k = 0, o = 0, r = 0, total = 0, rv;
  memset (&post, 0, 1024);
  memset (&db_query, 0, 1024);
  memset (&line, 0, 1024);



  for (i = 0; i < context->feed_count; i++)
    {
      ret =
	mrss_parse_url_with_options_and_error (context->feedlist[i], &data,
					       NULL, &code);


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
	      rv = sqlite3_prepare_v2 (h, db_query, -1, &statement, 0);
	      if (rv == SQLITE_OK)
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
			  for (k = 0; k < context->subscriber_count; k++)
			    {

			      push_message (context->subscribers[k], post);

			      //   printf ("~~> <%s>%s\n", post);



			    }
			}
		      else
			{
			  for (k = 0; k < context->subscriber_count; k++)
			    {
			      snprintf (post, 1024,
					"More feeds will be available later,please wait %d minutes for next update",
					WAIT_TIME / 60);
			      push_message (context->subscribers[k], post);
			      return;
			    }
			}


		    }

		}
	      else
		{
		  printf ("Rss Prepare error!! [%d],%s\n", rv, db_query);
		}
	    }

	}

    }
  mrss_free (data);
//   sqlite3_close (&h);
  return 0;

}
