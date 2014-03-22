#include <string.h>
#include <unistd.h>
#include <stdio.h>
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
  int i = 0,sz;
  sz=strlen(s);
  while (s[i] && i<sz)
    {
      s[i] = tolower (s[i]);
      ++i;
    }
  return s;
}

unsigned int
command (char *s)
{
  unsigned int i = 0 ;
  printf("$\n");
  for (i; i <= CMD_SIZE; ++i)
    {
     // printf("$$$-%s___%s***%d\n",&cmd_list[i][0], s, );
      fflush(stdout);
           if (strncmp (&cmd_list[i][0], s,strlen (&cmd_list[i][0])) == 0)
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
      
      if (strncmp (cmd_admin[i], s,strlen (&cmd_admin[i][0]) ) == 0)
	return i;

    }

  return 0;
}

void
kick (irc_session_t * session, context_t * context, char *query, char *sender,
      char *channel)
{
  char *who;
  who = strtok (query, " ");
  if (who == NULL)
    who = query;

  if ((strcmp (toLower (context->admin), toLower (who)) != 0)
      && (strcmp (toLower (context->name), toLower (who)) != 0))
    {
      irc_cmd_kick (session, who, channel, query);

    }
  else
    {
      irc_cmd_kick (session, sender, channel,
		    "That's cute,here's a real kick :)");


    }


}

unsigned int
isadmin (context_t * context, char *sender)
{

  if (strncmp (context->admin, sender, strlen (context->admin)) == 0)
    return 1;

  return 0;
}
