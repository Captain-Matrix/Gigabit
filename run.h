#ifndef run_h
#define run_h
#include "libircclient.h"

typedef struct
{
  int timeout;
  char command[1024];
  irc_session_t *session;
  char reply[256];
} runtime;
void *run (void *arg);
void rt_run (int t, char *to, char *channel, char *start_info, char *program,
	     char *args, irc_session_t * s);

#endif
