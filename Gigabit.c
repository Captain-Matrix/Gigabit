#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <lcfg/lcfg.h>

#include "libircclient.h"
#include "geo_ip.h"
#include "rss.h"
#include "run.h"
#include "cve.h"
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
  char cmd[256], query[1024], *who, redirect[1024], tmp[1024], dirty[1024];
  context_t *context = (context_t *) irc_get_ctx (session);
  memset (query, 0, 1024);
  memset (redirect, 0, 1024);
  memset (tmp, 0, 1024);
  memset (cmd, 0, 256);

  sz_msg = strlen (msg);
  memcpy (dirty, msg, 1024);
  if (sz_msg > 255 || sz_msg < 3)
    return;
  sanitize (msg, 0);
  sanitize (dirty, 1);
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
      if (contains (redirect, '#'))
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
  //replace (cmd, context->cmd, ' ');
  if (cmd[0] == context->cmd)
    snprintf (cmd, 256, "%s", &cmd[1]);
  trim (query);
  trim (&cmd[0]);
  toLower (&cmd[0]);

  printf ("[%s] $%s\n", cmd, query);
  rsz = strlen (redirect);
  //snprintf (cmd, 256, "%s", toLower (cmd));

  switch (command (cmd))
    {
    case 0:
      break;
    case CMD_HELP:
      push_message (channel,
		    "Command List: !help  !lookup !kick !version !quote , type $ to run any command as in \'$uname -a\' ");

      break;
    case CMD_LOOKUP:
      do_lookup (session, rsz ? redirect : channel, query);
      break;
    case CMD_KICK:
      kick (session, context, query, sender, channel);
      break;
    case CMD_VERSION:
      push_message (channel, VERSION);
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
//       start_cve_search (rsz ? redirect : sender,
//                      &dirty[contains (dirty, ' ')]);
//       push_message (channel, "Running cve search");
      push_message (channel, "Command not implemented yet :(  ");

      break;
    case CMD_SEEN:
      seen (context, channel, query);
      break;
    case CMD_DEFINE:

      rt_run (30, rsz ? redirect : sender, channel,
	      "Looking up definition ", "sdcv", query, session);
      break;
    default:
      rt_run(30,sender,channel,"Running command ",msg,NULL,session);
      break;

    }

  if (isadmin (context, sender))
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
	case ADMIN_RELOAD:
	  reload(120);
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
  sender_init ();

  printf ("connected %d\n", context->channel_count);
  fflush (stdout);
  //load_cve ("./allitems.txt");
  for (i; i < context->channel_count; i++)
    {
      irc_cmd_join (session, context->channels[i], 0);
      printf ("joining %s\n", context->channels[i]);
    }
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
  pthread_t sid ;

  pthread_create (&sid, 0, &send_thread, (void *) session);
  pthread_detach (sid);
 
//   pthread_t rid;
//   pthread_create (&rid, 0, ping, (void *) session);
//   pthread_detach (rid);
  pthread_t rcid;
  pthread_create (&rcid, 0, recycle, (void *) session);
  pthread_detach (rcid);
reload(120);
    context->runnerup=1;

  pthread_t tid ;
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
      pthread_t tid ;
      pthread_create (&tid, 0, &process_commands, (void *) &carg);
      pthread_detach (tid);

    }else if(strlen(params[1])> 1 && params[1][0] == '$'){
      cmd_args carg;		// = malloc (sizeof (cmd_args));
      carg.s = session;
      snprintf (carg.chnl, 256, "%s", params[0]);
      snprintf (carg.sndr, 256, "%s", buf);
     if(params[1][1]=='$'){
                    snprintf (carg.message, 2048, "%s 2>&1", &params[2]);

       rt_run(30,carg.chnl,carg.chnl,"Running command ",&carg.message[1],NULL,session); 

     }else{
             snprintf (carg.message, 2048, "%s 2>&1", params[1]);

       rt_run(30,carg.sndr,carg.chnl,"Running command ",&carg.message[1],NULL,session); 
     
       
    }
       
    }
}

void
onJoin (irc_session_t * session, const char *event, const char *origin,
	const char **params, unsigned int count)
{
  char buf[2048];

  irc_send_raw (session, "WHO %s", params[0]);
  snprintf (buf, 2048, "%s", &origin[contains (origin, '@') + 1]);
  //printf ("\%\% |%s|<%s>| \%\%\n", params[0], buf);
}

void
onPart (irc_session_t * session, const char *event, const char *origin,
	const char **params, unsigned int count)
{
  context_t *context = (context_t *) irc_get_ctx (session);
  user_left (context, params[0], origin);
}

void
onPrivmsg (irc_session_t * session, const char *event, const char *origin,
	   const char **params, unsigned int count)
{
  char buf[1024];
  snprintf (buf, contains (origin, '!') + 1, "%s", origin);

  if(strlen(params[1])> 1 && params[1][0] == '$'){
      cmd_args carg;		// = malloc (sizeof (cmd_args));
      carg.s = session;
      snprintf (carg.chnl, 256, "%s", params[0]);
      snprintf (carg.sndr, 256, "%s", buf);
      snprintf (carg.message, 2048, "%s 2>&1", params[1]);
     rt_run(30,carg.sndr,carg.chnl,"Running command ",&carg.message[1],NULL,session); 
    }
}

void
onNumeric (irc_session_t * session, unsigned int event,
	   const char *origin, const char **params, unsigned int count)
{
  char nickbuf[256];
  context_t *context = (context_t *) irc_get_ctx (session);
  sqlite3_stmt *statement;
  char db_query[1024];

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
      // printf ("~~~~~%d|%s %s|[%s] [%s] :%s:%s\n",count,origin,params[5], params[1], params[0],
      // params[2], params[3],params[4],params[6],params[7]);
      snprintf (db_query, 1024, "SELECT * FROM nickdb WHERE nick='%s'",
		params[5]);

      if (sqlite3_prepare_v2 (context->nickdb, db_query, -1, &statement, 0) ==
	  SQLITE_OK)
	{

	  //  printf("$$%s$$\n",db_query);

	  if (statement != NULL && (sqlite3_step (statement) == SQLITE_DONE))
	    {

	      snprintf (db_query, 1024,
			"INSERT INTO nickdb VALUES('%s','%s',0,%d)",
			params[5], params[3], time (NULL));
	      // printf("executing %s\n",db_query);
	      sqlite3_exec (context->nickdb, db_query, 0, 0, 0);

	    }
	}

    }
  else if (event == 366)
    {
      //  printf ("{%s} End of user list\n", params[1]);
    }
  else
    {
      printf ("\% [%d] <%s> %s %s %s\n", event, origin, params[0], params[1],
	      count > 3 ? params[3] : "");

    }
}

void
user_left (context_t * context, const char *channel, const char *nick)
{
  sqlite3_stmt *statement;
  char db_query[1024];

  snprintf (db_query, 1024, "SELECT * FROM nickdb WHERE nick='%s'", nick);

  if (sqlite3_prepare_v2 (context->nickdb, db_query, -1, &statement, 0) ==
      SQLITE_OK)
    {

      //  printf("$$%s$$\n",db_query);

      if (statement != NULL && (sqlite3_step (statement) == SQLITE_ROW))
	{

	  snprintf (db_query, 1024,
		    "INSERT INTO nickdb VALUES('%s','%s',0,%d)",
		    nick, sqlite3_column_text (statement, 1), time (NULL));
	  //   printf("executing %s\n",db_query);
	  sqlite3_exec (context->nickdb, db_query, 0, 0, 0);

	}
    }
}

void
seen (context_t * context, char *channel, char *nick)
{
  sqlite3_stmt *statement;
  char db_query[1024], msg[1024];
  int a;
  if (strlen (nick) < 1)
    return;

  snprintf (db_query, 1024, "SELECT * FROM nickdb WHERE nick='%s'", nick);
  printf ("Looking up hostname for nick |%s,sqlite query |%s\n", nick,
	  db_query);
  if (sqlite3_prepare_v2 (context->nickdb, db_query, -1, &statement, 0) ==
      SQLITE_OK)
    {


      if (statement != NULL && (sqlite3_step (statement) == SQLITE_ROW))
	{
	  sscanf (sqlite3_column_text (statement, 3), "%d", &a);
	  time_t t = (time_t) a;
	  snprintf (msg, 1024, "%s", ctime (&t));
	  push_message (channel, msg);
	  printf ("pushing %s to %s\n", msg, channel);
	  return;
	}
    }
  push_message (channel, "Nick not found,sorry.");
}

void
bot_connect (context_t * context)
{
  irc_callbacks_t cbs;
  static irc_session_t *session;
  memset (&cbs, 0, sizeof (cbs));
  cbs.event_connect = onConnect;
  cbs.event_join = onJoin;
  cbs.event_part = onPart;
  cbs.event_channel = onMessage;
  cbs.event_numeric = onNumeric;
  cbs.event_privmsg = onPrivmsg;
  session = irc_create_session (&cbs);
  irc_set_ctx (session, context);

  irc_option_set (session, LIBIRC_OPTION_SSL_NO_VERIFY);

  printf ("connecting to :%s\n", context->server);

  if (!context->v6)
    {
      if (irc_connect
	  (session, context->server, context->port, context->password,
	   context->nick, context->realname, VERSION))
	{
	  printf ("Could not connect:%s- %s\n", context->server,
		  irc_strerror (irc_errno (session)));
	  sleep (15);
	  printf ("Retrying connection to: %s\n", context->server);
	  bot_connect (context);
	}
    }
  else
    {
      if (irc_connect6
	  (session, context->server, context->port, context->password,
	   context->nick, context->realname, VERSION))
	{
	  printf ("Could not connect:%s- %s\n", context->server,
		  irc_strerror (irc_errno (session)));
	  sleep (15);
	  printf ("Retrying connection to: %s\n", context->server);
	  bot_connect (context);
	}

    }
  if (irc_run (session))
    {
      printf ("Could not connect or I/O error: %s:%d %s\n", context->server,
	      context->port, irc_strerror (irc_errno (session)));
sleep (5);
	  printf ("Retrying connection to: %s\n", context->server);
	  bot_connect (context);
    }

}

void
bot_start ()
{
  struct lcfg *c = lcfg_new ("./default.cfg");
  context_t *context = malloc (sizeof (context_t));
  memset (context, 0, sizeof (context_t));
  if (lcfg_parse (c) != lcfg_status_ok)
    {
      printf ("Error loading config file : %s\n", lcfg_error_get (c));
    }
  else
    {
      lcfg_accept (c, config_callback, context);

      if (context->network_count)
	{
	  bot_connect (context);
	}
//       else
//      exit (0);
    }
  lcfg_delete (c);
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
  if (getuid () == 0 || geteuid () == 0 || getgid () == 0)
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
