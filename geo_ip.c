#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "Gigabit.h"
#include "geo_ip.h"
#include "utilities.h"
#include "delayed_send.h"
static const char *
_mk_NA (const char *p)
{
  return p ? p : "N/A";
}

void
do_lookup (irc_session_t * session, char *to, char *args)
{
  geo_args *g_args = malloc (sizeof (geo_args));
  g_args->session = session;
  snprintf (g_args->dest, 256, "%s", to);
  snprintf (g_args->q, 256, "%s", args);
  pthread_t tid = (pthread_t) g_rand ();
  pthread_create (&tid, 0, &lookup, (void *) g_args);
  // pthread_join (tid, NULL);
}

void *
lookup (void *args)
{
  geo_args *g_args = (geo_args *) args;
  irc_session_t *session = g_args->session;
  char *dest = g_args->dest;
  char q[1024], info[256], nick_query[1024];
  snprintf (q, 1024, "%s", g_args->q);
  context_t *context = (context_t *) irc_get_ctx (session);
  sqlite3_stmt *nick_statement;

  if (strlen (q) < 3)
    return;

  if (!contains (q, '.'))
    {


      snprintf (nick_query, 1024, "SELECT * FROM nickdb WHERE nick='%s'", q);
      if (sqlite3_prepare_v2
	  (context->nickdb, nick_query, -1, &nick_statement, 0) == SQLITE_OK)
	{

	  if (sqlite3_step (nick_statement) == SQLITE_ROW)
	    {
	      snprintf (q, 1024, "%s",
			sqlite3_column_text (nick_statement, 1));
	    }
	}
    }
  snprintf (info, 256, "Performing GeoIP lookup of %s\n", q);
  push_message (dest, info);

  //////////////////////////////
  GeoIP *gi, *gi_isp;
  GeoIPRecord *gir;
  int generate = 0;
  char org[256], result[256];
  memset (result, 0, 256);
  strncpy (result, "Results unavailable at the moment.", 34);
  ;
  const char *time_zone = NULL;
  char **ret;

  gi = GeoIP_open (context->geo_ip_city, GEOIP_INDEX_CACHE);
  gi_isp = GeoIP_open (context->geo_ip_isp, GEOIP_STANDARD);

  if ((gi == NULL) || (gi_isp == NULL))
    {
      fprintf (stderr, "Error opening database\n");
      push_message (dest,
		    "Error opening geoip database,please make sure you have the right configuration for geo_ip_city and geo_ip_isp in your config");
      return;
    }




  gir = GeoIP_record_by_name (gi, q);


  if (gir != NULL)
    {
      ret = GeoIP_range_by_ip (gi, q);
      time_zone =
	GeoIP_time_zone_by_country_and_region (gir->country_code,
					       gir->region);
      snprintf (result, 1024,
		"%s>> Organizatoin:[%s] Country-code:[%s] Region:[%s %s] City:[%s] Postal-code:[%s] Lat:[%f] Long:[%f] Metro-code:[%d] Area-code:[%d] Time-zone:[%s]\n",
		q, _mk_NA (GeoIP_org_by_name (gi_isp, q)),
		_mk_NA (gir->country_code), _mk_NA (gir->region),
		_mk_NA (GeoIP_region_name_by_code
			(gir->country_code, gir->region)), _mk_NA (gir->city),
		_mk_NA (gir->postal_code), gir->latitude, gir->longitude,
		gir->metro_code, gir->area_code, _mk_NA (time_zone));
      GeoIP_range_by_ip_delete (ret);
      GeoIPRecord_delete (gir);
    }

  GeoIP_delete (gi);

  push_message (dest, result);
  free (args);
}
