#ifndef utilities_h
#define utilities_h
#include "Gigabit.h"
#include "libircclient.h"
static const char *safe =
  " \nABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_=,./@:#^*+-";

char *toLower (char *s);
void trim (char *s);
void sanitize (char *s);
unsigned int g_rand ();
unsigned int contains (const char *str, char c);
void replace (char *s, char a, char b);
unsigned int command (char *s);
unsigned int admin_cmd (char *s);
unsigned int isadmin (context_t * context, char *sender);
void kick (irc_session_t *  session, context_t * context, char *query,
	   char *sender, char *channel);
#endif
