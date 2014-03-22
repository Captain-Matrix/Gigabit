#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "Gigabit.h"
#include "geo_ip.h"
#include "delayed_send.h"
static const char *
_mk_NA (const char *p)
{
  return p ? p : "N/A";
}
void do_lookup(irc_session_t *session,char *to,char *args){
 geo_args * g_args = malloc (sizeof (geo_args));
      g_args->session = session;
      snprintf(g_args->dest,256,"%s", to);
      snprintf(g_args->q,256,"%s",args);
      pthread_t tid = (pthread_t) g_rand ();
      pthread_create (&tid, 0, &lookup, (void *) g_args);
      pthread_join (tid, NULL);
}
void *
lookup (void *args)
{
  geo_args *g_args = (geo_args *) args;
  irc_session_t *session = g_args->session;
  char *dest = g_args->dest;
  char *q = g_args->q;
  context_t *context = (context_t *) irc_get_ctx (session);
  sqlite3_stmt *nick_statement;

  if (strlen (q) < 3)
    return;

  char info[256];		/*,nick_query[1024];
				   snprintf (nick_query, 1024, "SELECT * FROM nickdb WHERE nick='%s'",
				   q);
				   if (sqlite3_prepare_v2 (context->nickdb, nick_query, -1, &nick_statement, 0) ==
				   SQLITE_OK)
				   {

				   if (sqlite3_step (nick_statement) == SQLITE_ROW)
				   {
				   printf("lookup nick matcing query ==%s\n",sqlite3_column_name (nick_statement, 2));
				   }
				   } */
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

  gi = GeoIP_open ("./data/GeoIPCity.dat", GEOIP_INDEX_CACHE);
  gi_isp = GeoIP_open ("./data/GeoIPISP.dat", GEOIP_STANDARD);

  if ((gi == NULL) || (gi_isp == NULL))
    {
      fprintf (stderr, "Error opening database\n");
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
		"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%f\t%f\t%d\t%d\t%s\n", q,
		_mk_NA (GeoIP_org_by_name (gi_isp, q)),
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

}
