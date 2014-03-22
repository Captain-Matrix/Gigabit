#include "GeoIP.h"
#include "GeoIPCity.h"
#include "libircclient.h"
#include <sqlite3.h>
#ifndef GEO_IP_H
#define GEO_IP_H

void *lookup (void *g_args);
typedef struct
{
  irc_session_t *session;
  char dest[256];
  char q[1024];
} geo_args;
void do_lookup(irc_session_t *session,char *to,char *args);
#endif
