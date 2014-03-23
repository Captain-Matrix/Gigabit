#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <lcfg/lcfg.h>
#include <stdlib.h>
#include "utilities.h"
#include "Gigabit.h"
#include "libircclient.h"

void
replace (char *s, char a, char b)
{
  unsigned int i = 0, sz = strlen (s);
  for (i; i < sz; i++)
    {
      if (s[i] == a)
	s[i] = b;
    }
}

unsigned int
contains (const char *str, char c)
{
  int i = 0, sz = strlen (str);
  if (sz < 1)
    return 0;
  for (i; i < sz; i++)
    if (str[i] == c)
      return i ? i : 1;

  return 0;
}

unsigned int
g_rand ()
{
  srand (time (NULL));
  return rand () % 100 + 1;
}

void
sanitize (char *s)
{
  int sz = strlen (s), sz_al = strlen (safe), i = 0, j = 0, ok = 0;
  if (sz < 1)
    return;
  for (i = 0; i < sz; i++)
    {
      for (j = 0; j < sz_al; j++)
	{
	  if (s[i] == safe[j])
	    {
	      ok = 1;
	      break;
	    }

	}
      if (!ok)
	s[i] = ' ';
      ok = 0;
    }
}

void
trim (char *s)
{
  char *string = s;
  int sz = strlen (string);

  if (sz < 3)
    return;
  while (isspace (string[0]))
    ++string;			//get rid of leading white space

  sz = strlen (string);
  while ((string[sz - 1] == ' ') || (string[sz - 1] == '\n'))
    {
      string[sz - 1] = '\0';	//get rid of trailing white space
      --sz;
    }


}

char *
toLower (char *s)
{
  int i = 0, sz;
  sz = strlen (s);
  while (s[i] && i < sz)
    {
      s[i] = tolower (s[i]);
      ++i;
    }
  return s;
}

unsigned int
command (char *s)
{
  unsigned int i = 0;
  printf ("$\n");
  for (i; i <= CMD_SIZE; ++i)
    {
      // printf("$$$-%s___%s***%d\n",&cmd_list[i][0], s, );
      fflush (stdout);
      if (strncmp (&cmd_list[i][0], s, strlen (&cmd_list[i][0])) == 0)
	return i;

    }

  return 0;
}

unsigned int
admin_cmd (char *s)
{

  unsigned int i = 0;
  for (i; i <= ADMIN_SIZE; ++i)
    {

      if (strncmp (cmd_admin[i], s, strlen (&cmd_admin[i][0])) == 0)
	return i;

    }

  return 0;
}

void
kick (irc_session_t * session, context_t * context, char *query, char *sender,
      char *channel)
{
  char *who;
  int i = 0;
  who = strtok (query, " ");
  if (who == NULL)
    who = query;
  toLower (who);
  toLower (sender);
  for (i; i < context->admin_count; i++)
    {
      if (strncmp (context->admins[i], sender, strlen (context->admins[i])) ==
	  0)
	{
	  irc_cmd_kick (session, sender, channel,
			"That's cute,here's a real kick :)");
	  return;
	}
    }
  irc_cmd_kick (session, who, channel, query);
}

unsigned int
isadmin (context_t * context, char *sender)
{
  int i = 0;
  for (i; i < context->admin_count; i++)
    {
      if (strncmp (context->admins[i], sender, strlen (context->admins[i])) ==
	  0)
	return 1;
    }
  return 0;
}

enum lcfg_status
config_callback (const char *key, void *data, size_t len, void *user_data)
{
  char *s = data;
  context_t *context = user_data;
  if (strncmp ("network.server", key, 14) == 0)
    {
      if (fork ())
	return;
      context->network_count++;
      snprintf (context->server, 256, "%s", data);

    }
  else if (strncmp ("network.port", key, 12) == 0)
    {
      context->port = atoi (s);
    }
  else if (strncmp ("network.autojoin", key, 16) == 0)
    {
      snprintf (context->channels[context->channel_count], 256, "%s", s);
      ++context->channel_count;
    }
  else if (strncmp ("network.rss_subscribers", key, 23) == 0)
    {
      snprintf (context->subscribers[context->subscriber_count], 256, "%s",
		s);
      ++context->subscriber_count;
    }
  else if (strncmp ("network.admins", key, 14) == 0)
    {
      snprintf (context->admins[context->admin_count], 256, "%s", s);
      ++context->admin_count;
    }
  else if (strncmp ("network.feed", key, 13) == 0)
    {
      snprintf (context->feedlist[context->feed_count], 256, "%s", s);
      ++context->feed_count;
    }
  else if (strncmp ("network.realname", key, 16) == 0)
    {
      snprintf (context->realname, 256, "%s", s);
    }
  else if (strncmp ("network.password", key, 16) == 0)
    {
      snprintf (context->password, 256, "%s", s);
    }
  else if (strncmp ("network.geo_ip_isp", key, 16) == 0)
    {
      snprintf (context->geo_ip_isp, 256, "%s", s);
    }
  else if (strncmp ("network.geo_ip_city", key, 16) == 0)
    {
      snprintf (context->geo_ip_city, 256, "%s", s);
    }
  else if (strncmp ("network.nick", key, 12) == 0)
    {
      snprintf (context->nick, 256, "%s", s);
    }
  else if (strncmp ("network.rss_db_path", key, 19) == 0)
    {
      snprintf (context->rss_db, 256, "%s", s);
    }
  else if (strncmp ("network.ipv6", key, 12) == 0)
    {
      context->v6 = strncmp ("enable", s, 6) ? 0 : 1;
    }
  else if (strncmp ("network.command", key, 15) == 0)
    {

      context->cmd = s[0];
    }
  return lcfg_status_ok;
}
