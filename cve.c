#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <regex.h>
#include "Gigabit.h"
#include "delayed_send.h"
#include "cve.h"


pthread_mutex_t c_lock;

int
load_cve (char *path)
{

  FILE *f = fopen (path, "r");
  int i = 0, sz;
  char line[1024];
  if (!f)
    return 1;
  CIRCLEQ_INIT (&head);
  while (!feof (f))
    {
      fgets (line, 1024, f);
      if (strncmp (line, DELIMITER, 10) == 0)
	{
	  entry *en = malloc (sizeof (entry));
	  for (i = 0, sz = 0; i < 20 && !feof (f); i++)
	    {
	      fgets (line, 1024, f);
	      sz += strlen (line);
	      if (strncmp (line, DELIMITER, 10) == 0)
		{

		  CIRCLEQ_INSERT_HEAD (&head, en, entries);

		  break;
		}
	      if (line)
		{
		  en->buf[i] = malloc (strlen (line) + 1);
		  ++en->count;
		  memcpy (en->buf[i], &line, strlen (line) + 1);
		}
	    }

	}



    }
  fclose (f);

  return 0;

}

void
start_cve_search (char *destination, char *query)
{
  cve_query *cq = malloc (sizeof (cve_query));
  snprintf (cq->destination, 256, "%s", destination);
  snprintf (cq->query, 256, "%s", query);
  pthread_t t;
  pthread_create (&t, 0, &cve_searcher, (void *) cq);
  pthread_detach (t);

}

void *
cve_searcher (void *arg)
{
  int i = 0, j = 0, k = 0, total = 0, count = 0, sz;
  char buf[20][LINE_SIZE_MAX], line[LINE_SIZE_MAX];
  cve_query *cq = arg;
  regex_t rx;
  regmatch_t m[1];
  FILE *f = fopen ("./allitems.txt", "r");
  if (f == NULL)
    return;



  if (regcomp (&rx, cq->query, REG_ICASE | REG_EXTENDED | REG_NOSUB) != 0)
    {
      printf ("regex compilation failed!!\n");
      push_message (cq->destination, "Invalid regular expression provided");
      return;

    }
  if (pthread_mutex_lock (&c_lock) == 0)
    {
      printf ("Executing cve query of %s destined for %s\n", cq->query,
	      cq->destination);
      while (!feof (f))
	{
	  fgets (line, LINE_SIZE_MAX, f);
	  if (strlen (line) > 10)
	    if (strncmp (line, DELIMITER, strlen (line)) == 0)
	      {
		for (i = 0, sz = 0; i < 20 && !feof (f);)
		  {
		    fgets (line, LINE_SIZE_MAX, f);
		    sz += strlen (line);
		    if (sz > 10)
		      if (strncmp (line, DELIMITER, sz) == 0)
			{
			  for (j = 0; j < count; j++)
			    {
			      if (!regexec (&rx, buf[j], 1, m, 0))
				{
				  printf
				    ("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
				  fflush (stdout);
				  for (i = 0; i < count; i++)
				    {
				      if (buf[0])
					printf ("--%s", buf[0]);
				      fflush (stdout);
				      push_message (cq->destination, buf[i]);

				      ++total;
				      if (total > 50)
					return;
				    }
				}
			    }
			  count = 0;
			  goto next;
			}
		    if (line)
		      {
			count++;

			snprintf (buf[i], strlen (line), "%s", &line);
			++i;
		      }
		  }

	      }
	next:
	  i;
	}
    }

  pthread_mutex_unlock (&c_lock);
  regfree (&rx);
  free (cq);
}
