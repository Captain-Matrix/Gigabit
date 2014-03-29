#ifndef cve_h
#define cve_h
#include <sys/queue.h>
#include <sys/types.h>
#define DELIMITER "======================================================"
#define LINE_SIZE_MAX 256
struct cve_query
{
  char destination[256];
  char query[256];
};
typedef struct cve_query cve_query;
struct entry
{
  char *buf[20];
  unsigned char count;
    CIRCLEQ_ENTRY (entry) entries;
};
typedef struct entry entry;
CIRCLEQ_HEAD (queue, entry) head;
     void start_cve_search (char *destination, char *query);
     void *cve_searcher (void *arg);
     int load_cve (char *path);
#endif
