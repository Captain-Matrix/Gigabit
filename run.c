#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "run.h"
#include "Gigabit.h"
#include "delayed_send.h"


void *
run (void *arg)
{
  runtime *rt = (runtime *) arg;
//context_t *context=(context_t*) irc_get_ctx(rt->session);

  FILE *fp = popen (rt->command, "r");
  char buf[256], big_buf[256][256], destination[256], c;
  int to = rt->timeout;
  time_t start = time (NULL), last, now;
  snprintf (destination, 256, "%s", rt->reply);
  free (rt);
  //printf ("##%s##%s##%s##\n", destination, rt->reply, rt->command);
  if (fp == NULL)
    {
      printf ("command error\n");
      return;
    }

  int flags, fd = fileno (fp), sent = 0, i, line_count=0;

  flags = fcntl (fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl (fd, F_SETFL, flags);
  printf ("run thread started\n");
  last = time (NULL);
  do
    {

      c = fgetc (fp);

      if (c > 0 && c < 255)
	{
	  ungetc (c, fp);

	  fgets (buf, 255, fp);
	  if (buf[strlen (buf) - 1] == '\n')
	    buf[strlen (buf) - 1] = '\0';
	  // trim (buf);
	  if (strlen (buf) > 0)
	    {
	      push_message (destination, buf);
	      ++line_count;
	    }
	}


    }
  while (time (NULL) < (start + to) && line_count <= 50);
  pclose (fp);

  return;
}

void
rt_run (int t, char *to, char *channel, char *start_info, char *program,
	char *args, irc_session_t * s)
{
  char tmp[256];
  pthread_t tid = (pthread_t) g_rand ();
  runtime *rt = malloc (sizeof (runtime));
  rt->timeout = t;

  snprintf (rt->reply, 256, "%s", to);
  rt->session = s;
  if (start_info)
    {
      snprintf (tmp, 256,
		"%s,Output will be messaged to %s", start_info, rt->reply);
      push_message (channel, tmp);
    }
  if (args != NULL)
    snprintf (rt->command, 1024, "timeout -sSIGKILL %d %s %s", rt->timeout,
	      program, args);
  else
    snprintf (rt->command, 1024, "timeout -sSIGKILL %d %s", rt->timeout,
	      program);

  printf ("running \"%s\"\n", rt->command);
  pthread_create (&tid, 0, &run, (void *) rt);
  pthread_detach (tid);
  // pthread_join (tid, NULL);

}

/*
int
main (int argc, char **argv)
{
  pthread_t tid;
  runtime *rt = malloc (sizeof (runtime));
  rt->timeout = 10;
  rt->command = malloc (1024);
  sprintf (rt->command, "timeout -sSIGKILL %d find ~", rt->timeout);

  pthread_create (&tid, 0, &run, (void *) rt);
  pthread_join (tid, NULL);

  free (rt->command);
  free (rt);
  return 0;
}*/
