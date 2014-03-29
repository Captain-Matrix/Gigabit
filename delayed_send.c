//Delayed send,goal is to pipe all messages to this thread so it can
//prevent itself from flooding the server and gettick k-lined by the ircd

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include "libircclient.h"
#include "delayed_send.h"


TAILQ_HEAD (, message) message_head;
     irc_session_t *session;
     message *queue;
     pthread_mutex_t t_lock;
     unsigned int counter = 0;

     pthread_cond_t more;

     void *send_thread (void *arg)
{
  session = arg;
  while (1)
    {


      if (pthread_mutex_lock (&t_lock) == 0)
	{
	  if (counter < 1)
	    {
	      pthread_cond_wait (&more, &t_lock);
	    }

	  TAILQ_FOREACH (queue, &message_head, entries)
	  {
	    //  libirc_mutex_lock (session->mutex_session);
	    if (queue != NULL)
	      {
		printf (">>> <%s>%s\n", queue->destination, queue->data);
		fflush (stdout);
		irc_cmd_msg (session, queue->destination, queue->data);

		TAILQ_REMOVE (&message_head, queue, entries);
		free (queue);
		//libirc_mutex_unlock (session->mutex_session);

	      }
	    pthread_mutex_unlock (&t_lock);

	    sleep (2);

	    --counter;
	  }

	}


    }

}

void
push_message (char *dest, char *msg)
{
  if (strlen (msg) < 1 || strlen (dest) < 2)
    return;
  if (pthread_mutex_lock (&t_lock) == 0)
    {
      queue = malloc (sizeof (message));
      memset (queue, 0, sizeof (message));
      if (queue != NULL)
	{
	  snprintf (queue->destination, 1024, "%s", dest);
	  snprintf (queue->data, 1024, "%s", msg);
	  TAILQ_INSERT_TAIL (&message_head, queue, entries);

	  ++counter;
	  pthread_cond_signal (&more);

	}
      pthread_mutex_unlock (&t_lock);

    }
}

void
sender_init ()
{
  TAILQ_INIT (&message_head);
  pthread_mutex_init (&t_lock, NULL);
  pthread_cond_init (&more, NULL);

}

/*
unsigned int
g_rand ()
{
  srand (time (NULL));
  return rand () % 1000000 + 1;
}

void *
test_threads (void *args)
{
  char m[1024], d[1024];
  int i;
  for (i = 0; i < 100; i++)
    {
      sprintf (d, "<%d>", i);
      sprintf (m, "[%d]\n", g_rand ());
      push_message (d, m);
      usleep(10000);
    }
}

int
main ()
{
  pthread_t rid, sid = (pthread_t) g_rand ();
  int i = 0;
  TAILQ_INIT (&message_head);
  pthread_mutex_init (&t_lock, NULL);
  pthread_cond_init(&more,NULL);
  pthread_create (&sid, 0, &send_thread, NULL);
  for (i; i < 100; i++)
    {
      rid = i;
      pthread_create (&rid, 0, &test_threads, NULL);

    }
  pthread_join (sid, NULL);
}*/
