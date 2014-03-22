#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>

#include "libircclient.h"
#include "geo_ip.h"
#include "rss.h"
#include "run.h"
#include "Gigabit.h"
#include "delayed_send.h"
#include "utilities.h"





void *
process_commands (void *args)
{
  cmd_args *carg = (cmd_args *) args;

  irc_session_t *session = carg->s;
  char *sender = carg->sndr;
  char *channel = carg->chnl;
  char *msg = carg->message;
  int sz_cmd, sz_msg, rsz, w;
  char cmd[256], query[1024], *who, redirect[1024], tmp[1024];
  context_t *context = (context_t *) irc_get_ctx (session);
  memset (query, 0, 1024);
  memset (redirect, 0, 1024);
  memset (tmp, 0, 1024);
  memset (cmd, 0, 256);

  sz_msg = strlen (msg);

  if (sz_msg > 255 || sz_msg < 3)
    return;
  sanitize (msg);

  if (contains (msg, '@'))
    {
      sscanf (msg, "%s %s %s%n", cmd, redirect, query, &w);
    }
  else
    sscanf (msg, "%s %s%n", cmd, query, &w);
  if (strlen (query) > 0 && w < strlen (msg))
    {
      snprintf (query, 1024, "%s", msg + (w - strlen (query)));
    }
  if (strlen (redirect) > 0)
    {
      replace (redirect, '@', ' ');
      trim (redirect);
      if (contains(redirect,'#'))
	{
	  push_message (channel,
			"Redirecting output to a channel not supported.");
	  return;
	}
    }
  if (strlen (query) < 1)
    snprintf (query, 1024, "none");

  if (strlen (cmd) < 2)
    return;
  replace (cmd, context->cmd, ' ');

  trim (query);

  printf ("[%s] $%s\n", cmd, query);
  rsz = strlen (redirect);
  //snprintf (cmd, 256, "%s", toLower (cmd));
  toLower(&cmd[0]);
  switch (command (cmd))
    {
    case 0:
      break;
    case CMD_HELP:
      push_message (channel,
		    "Command List: !help  !lookup !kick !version !quote !define !nmap  !calc !ping !traceroute !man !dig !subnet !cc !cve-search !seen ");

      break;
    case CMD_LOOKUP:
do_lookup(session,rsz ? redirect : channel,query);
break;
    case CMD_KICK:
      kick (session, context, query, sender, channel);
      break;
    case CMD_VERSION:
      push_message (channel, context->version);
      break;
    case CMD_QUOTE:
      rt_run (30, rsz ? redirect : channel, channel,
	      NULL, "fortune -ae", NULL, session);
      break;
    case CMD_NMAP:
      rt_run (120, rsz ? redirect : sender, channel,
	      " Started nmap scan", "nmap", query, session);
      break;
    case CMD_CALC:
      replace (query, '_', '/');
      rt_run (120, rsz ? redirect : channel, channel,
	      NULL, "bc -q <<< ", query, session);
      break;
    case CMD_PING:
      rt_run (60, rsz ? redirect : sender, channel,
	      "Pinging host ", "ping", query, session);
      break;
    case CMD_TRACEROUTE:
      rt_run (30, rsz ? redirect : sender, channel,
	      "Running Traceroute ", "traceroute", query, session);
      break;
    case CMD_MAN:
      rt_run (60, rsz ? redirect : sender, channel,
	      "Looking up Manual ", "man -a", query, session);
      break;
    case CMD_DIG:
      rt_run (30, rsz ? redirect : sender, channel,
	      "Looking up DNS info", "dig", query, session);
      break;
    case CMD_SUBNET:
      rt_run (120, rsz ? redirect : sender, channel,
	      "Calculating subnet mask information", "sipcalc", query,
	      session);
      break;
    case CMD_CC:
      push_message (channel, "Command not implemented yet :(  ");

      break;
    case CMD_CVE:
      push_message (channel, "Command not implemented yet :(  ");

      break;
    case CMD_SEEN:
      push_message (channel, "Command not implemented yet :(  ");

      break;
    case CMD_DEFINE:

      rt_run (30, rsz ? redirect : sender, channel,
	      "Looking up definition ", "sdcv", query, session);
      break;
    default:
      printf("NO MATCH FOR COMMAND %d",command(cmd));
      break;

    }

  if (isadmin (context,sender))
    {
      switch (admin_cmd (cmd))
	{
	case 0:
	  break;
	case ADMIN_TOPIC:
	  irc_cmd_topic (session, channel, query);
	  break;
	case ADMIN_PART:
	  irc_cmd_part (session, channel);
	  context->channel_count--;
	  break;
	case ADMIN_QUIT:
	  irc_cmd_quit (session, "Quitting due to admin request");
	  sleep (2);
	  exit (0);
	  break;
	case ADMIN_JOIN:
	  irc_cmd_join (session, query, NULL);
	  snprintf (context->channels[++context->channel_count], 64, "%s",
		    query);
	  context->channel_count++;
	  break;
	case ADMIN_NICK:
	  irc_cmd_nick (session, query);
	  break;
	default:
	  break;

	}
    }

  return;
}

void
onConnect (irc_session_t * session, const char *event, const char *origin,
	   const char **params, unsigned int count)
{

  int i = 0;
  context_t *context = (context_t *) irc_get_ctx (session);
  printf ("connected %d\n", context->channel_count);
  fflush (stdout);
  if (sqlite3_open (context->nick_db, &context->nickdb))
    {
      printf ("Error opening nick db\n");
      fflush (stdout);
    }
  else
    {

      sqlite3_exec (context->nickdb,
		    "CREATE TABLE IF NOT EXISTS nickdb (nick TEXT PRIMARY KEY,hostname TEXT NOT NULL,linecount INTEGER,lastseen INTEGER)",
		    0, 0, 0);
      context->db_nick = 1;
    }
  sender_init ();
  pthread_t sid = (pthread_t) g_rand ();

  pthread_create (&sid, 0, &send_thread, (void *) session);
  pthread_detach (sid);
  for (i; i < context->channel_count; i++)
    {
      irc_cmd_join (session, context->channels[i], 0);
      printf ("joining %s\n", context->channels[i]);
    }

  pthread_t tid = (pthread_t) g_rand ();
  pthread_create (&tid, 0, &run_rss, (void *) session);
  pthread_detach (tid);

}

void
onMessage (irc_session_t * session, const char *event, const char *sender,
	   const char **params, unsigned int count)
{
  context_t *context = (context_t *) irc_get_ctx (session);
  char buf[2048], buf2[2048];
  snprintf (buf, contains (sender, '!') + 1, "%s", sender);
  printf ("<<< %s <%s>%s\n", params[0], buf, params[1]);
  fflush (stdout);
  if (strlen (params[1]) > 0 && params[1][0] == context->cmd)
    {
      cmd_args carg;		// = malloc (sizeof (cmd_args));
      carg.s = session;
      snprintf (carg.chnl, 256, "%s", params[0]);
      snprintf (carg.sndr, 256, "%s", buf);
      snprintf (carg.message, 2048, "%s", params[1]);
      //process_commands(&carg);
      pthread_t tid = (pthread_t) g_rand ();
      pthread_create (&tid, 0, &process_commands, (void *) &carg);
      pthread_detach (tid);

    }
}

void
onJoin (irc_session_t * session, const char *event, const char *origin,
	const char **params, unsigned int count)
{
  char buf[2048];

  irc_send_raw (session, "WHO %s", params[0]);
  snprintf (buf, 2048, "%s", &origin[contains (origin, '@') + 1]);
  printf ("\%\% |%s|<%s>| \%\%\n", params[0], buf);
}

void
onPrivmsg (irc_session_t * session, const char *event, const char *origin,
	   const char **params, unsigned int count)
{
  printf ("\%%s <%s>%s\n", origin, params[0], params[1]);

}

void
onNumeric (irc_session_t * session, unsigned int event,
	   const char *origin, const char **params, unsigned int count)
{
  char nickbuf[256];
  context_t *context = (context_t *) irc_get_ctx (session);

  if (event > 400)
    {
      if (event == 433)
	{
	  snprintf (nickbuf, 256, "%s_", context->nick);
	  irc_cmd_nick (session, nickbuf);
	  snprintf (context->nick, 256, "%s", nickbuf);
	}
      printf ("ERROR %d: %s: %s %s %s %s\n",
	      event,
	      origin ? origin : "unknown",
	      params[0],
	      count > 1 ? params[1] : "",
	      count > 2 ? params[2] : "", count > 3 ? params[3] : "");
    }
  else if (event == 353)
    {
      printf ("@@|%s|\n", params[3]);
    }
  else if (event == 352)
    {
      printf ("%s|[%s] [%s] :%s:\n", params[1], params[0],
	      params[2], params[3]);

    }
  else if (event == 366)
    {
      printf ("{%s} End of user list\n", params[1]);
    }
  else
    {
      printf ("\% [%d] <%s> %s %s %s\n", event, origin, params[0], params[1],
	      count > 3 ? params[3] : "");

    }
}

void
bot_connect (char *confpath)
{
  irc_callbacks_t cbs;
  static irc_session_t *session;
  context_t *context = malloc (sizeof (context_t));
  memset (context, 0, sizeof (context_t));
  FILE *cns, *rssi;
  FILE *config;
  char *line, *p1, *p2;
  int i = 0, v6 = 0;
  line = malloc (256);
  memset (line, 0, 256);

  config = fopen (confpath, "r");
  if (!config)
    {
      printf ("Unable to open configuration for %s\n", confpath);
      exit (1);
    }
  while (!feof (config))
    {

      fgets (line, 255, config);
      if (line)
	{
	  trim (line);
	  if (line[0] != '#')
	    {
	      p1 = strtok (line, "=");
	      p2 = p1 + (strlen (p1) + 1);
	      //printf("%s --> %s\n",p1,p2);
	      p1 = toLower (p1);
	      trim (p2);
	      if (strcmp (p1, "admin") == 0)
		{
		  snprintf (context->admin, 256, "%s", p2);
		}
	      if (strcmp (p1, "command") == 0)
		{
		  context->cmd = p2[0];
		}
	      if (strcmp (p1, "server") == 0)
		{
		  snprintf (context->server, 256, "%s", p2);
		}
	      if (strcmp (p1, "username") == 0)
		{
		  snprintf (context->name, 256, "%s", p2);
		}
	      if (strcmp (p1, "nick") == 0)
		{
		  snprintf (context->nick, 256, "%s", p2);
		}
	      if (strcmp (p1, "password") == 0)
		{
		  snprintf (context->password, 256, "%s", p2);
		}
	      if (strcmp (p1, "version") == 0)
		{
		  snprintf (context->version, 64, "%s", p2);
		}
	      if (strcmp (p1, "rss_db") == 0)
		{
		  snprintf (context->rss_db, 256, "%s", p2);
		}
	      if (strcmp (p1, "nick_db") == 0)
		{
		  snprintf (context->nick_db, 256, "%s", p2);
		}

	      if (strcmp (p1, "port") == 0)
		{
		  context->port = atoi (p2);
		}
	      if (strcmp (p1, "autojoin") == 0)
		{
		  if ((cns = fopen (p2, "r")) == NULL)
		    {
		      printf
			("Error loading channel list or config file\n\r");
		      return;
		    }
		}
	      if (strcmp (p1, "subscribers") == 0)
		{
		  snprintf (context->subscribers, 256, "%s", p2);
		}
	      if (strcmp (p1, "ipv6") == 0)
		{
		  v6 = atoi (p2);
		}
	    }
	}
    }

  fclose (config);
  for (i = 0; i < 256 && !feof (cns); i++)
    {
      fgets (context->channels[i], 32, cns);
      context->channels[i][strlen (context->channels[i]) - 1] = '\0';
    }
  context->channel_count = i - 2;


  memset (&cbs, 0, sizeof (cbs));
  cbs.event_connect = onConnect;
  cbs.event_join = onJoin;
  cbs.event_channel = onMessage;
  cbs.event_numeric = onNumeric;
  cbs.event_privmsg = onPrivmsg;
  session = irc_create_session (&cbs);

  irc_set_ctx (session, context);

  irc_option_set (session, LIBIRC_OPTION_SSL_NO_VERIFY);
  printf ("connecting to :%s\n", context->server);

  if (!v6)
    {
      if (irc_connect
	  (session, context->server, context->port, context->password,
	   context->name, context->nick, context->version))
	{
	  printf ("Could not connect:%s- %s\n", context->server,
		  irc_strerror (irc_errno (session)));
	}
    }
  else
    {
      if (irc_connect6
	  (session, context->server, context->port, context->password,
	   context->name, context->nick, context->version))
	{
	  printf ("Could not connect:%s- %s\n", context->server,
		  irc_strerror (irc_errno (session)));
	}

    }
  if (irc_run (session))
    {
      printf ("Could not connect or I/O error: %s:%d %s\n", context->server,
	      context->port, irc_strerror (irc_errno (session)));

    }
  free (line);

}

void
bot_start ()
{
  int i = 0;
  char line[1024];
  pid_t pid;
  FILE *nets = fopen ("./config", "r");
  if (nets == NULL)
    exit (1);
  for (i = 0; !feof (nets); i++)
    {
      fgets (line, 1024, nets);
      if (line[strlen (line) - 1] == '\n')
	line[strlen (line) - 1] = '\0';
      trim (line);
      if (strlen (line) > 2)
	{
	  pid = fork ();
	  if (pid == 0)
	    bot_connect (line);
	  else
	    printf ("Started %d children for network %s PID %d\n", i, line,
		    pid);
	}
    }
  if (pid)
    fclose (nets);
  printf ("Child bots running,parent committing suicide....\n");
}

void
CATCH_ALL (int signal)
{
  printf
    ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^SIGNAL(%d) CAUGHT^^^^^^^^^^^^^^^^^^^^^^^^\n",
     signal);
  fflush (stdout);
  switch (signal)
    {
    case SIGSEGV:
      printf ("Of all the things I've lost I miss my mind the most.\a\n");
      exit (0xDEAD);
      break;
    case SIGKILL:
      printf ("They killed me :( !!!\n");
      break;
    case SIGCHLD:
      printf ("Child process exited.\n");
      break;
    default:
      printf ("Houston We have a problem!!\n");
      return;
      break;
    }
}

void
safe_to_run ()
{
  if (getuid == 0 || geteuid == 0 || getgid () == 0)
    {
      printf
	("Really?running an irc bot as root??shame shame shame...tsk...tsk...tsk!!\a\n");
      exit (0xDEAD);
    }

  setreuid (65534, 65533);
  setregid (65534, 65533);
  setuid (65534);
  setgid (65533);
}

int
main (int argc, char **argv)
{
  int i = 0;
  safe_to_run ();
  for (i; i < 32; i++)
    signal (i, CATCH_ALL);
  bot_start ();
//   bot_connect ("./freenode.network");
  return 0;
}
