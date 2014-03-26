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
  int i = 0;
  char line[1024];
  if (!f)
    return 1;
  CIRCLEQ_INIT (&head);
  while (!feof (f))
    {
      fgets (line, 1024, f);
      if (strncmp (line, DELIMITER, 10) == 0)
	{
	  entry *en = malloc (sizeof (struct entry));

	  for (i = 0; i < 20 && !feof (f); i++)
	    {
	      fgets (line, 1024, f);

	      if (strncmp (line, DELIMITER, 10) == 0)
		{

		  CIRCLEQ_INSERT_HEAD (&head, en, entries);

		  break;
		}
	      snprintf (en->buf[i], LINE_SIZE_MAX, "%s", line);

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
  int i = 0, total = 0;
  cve_query *cq = arg;
  entry *e;
  regex_t rx;
  regmatch_t m[1];
  printf ("Executing cve query of %s destined for %s\n", cq->query,
	  cq->destination);
  if (regcomp (&rx, cq->query, REG_ICASE) != 0)
    {
      printf ("regex compilation failed!!\n");
      push_message (cq->destination, "Invalid regular expression provided");
      return;

    }
  if (pthread_mutex_lock (&c_lock) == 0)
    {

      for (e = head.cqh_last; e != (void *) &head; e = e->entries.cqe_prev)
	{
// fgetc(stdin);

	  for (i = 0; i < 20; i++)
	    {
	      if (!regexec (&rx, e->buf[i], 1, m, 0))
		{
		  printf
		    ("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		  for (i = 0; i < 20; i++)
		    {
		      push_message (cq->destination, e->buf[i]);
		      printf ("%s", e->buf[i]);
		      fflush (stdout);
		      ++total;
		      if (total > 50)
			break;
		    }

		}

	    }
	}
    }
  pthread_mutex_unlock (&c_lock);
  regfree (&rx);
  free (cq);
}
