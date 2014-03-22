#ifndef Gigabit_h
#define Gigabit_h
#include <sqlite3.h>
#include "libircclient.h"
#define CMD_HELP 1
#define CMD_LOOKUP 2
#define CMD_KICK 3
#define CMD_VERSION 4
#define CMD_QUOTE 5
#define CMD_NMAP 6
#define CMD_CALC 7
#define CMD_PING 8
#define CMD_TRACEROUTE 9
#define CMD_MAN 10
#define CMD_DIG 11
#define CMD_SUBNET 12
#define CMD_CC 13
#define CMD_CVE 14
#define CMD_SEEN 15
#define CMD_DEFINE 16
///////////////////////
#define CMD_SIZE 16
//////////////////////
#define ADMIN_TOPIC 1
#define ADMIN_PART 2
#define ADMIN_QUIT 3
#define ADMIN_JOIN 4
#define ADMIN_NICK 5
//////////////////
#define ADMIN_SIZE 5
/////////////////
static const char cmd_list[17][15] =
  { "NA", "help", "lookup", "kick", "version", "quote", "nmap", "calc",
  "ping", "traceroute", "man", "dig", "subnet", "cc", "cve-search", "seen",
  "define"
};

static const char cmd_admin[6][15] = { "NA", "topic", "part", "quit", "join", "nick" };

typedef struct
{
  char nick[256];
  char name[256];
  char admin[256];
  char password[256];
  char server[256];
  char version[64];
  unsigned short port;
  char channels[256][64];
  char subscribers[256];
  char nick_db[256];
  char rss_db[256];
  sqlite3 *nickdb, *replydb;

  int ssl;
  int v6;
  int db_nick;
  int channel_count;
  int rss_ignore_count;
  char cmd;
} context_t;
////////////////////////////////////////////
typedef struct
{
  irc_session_t *s;
  char sndr[256];
  char chnl[256];
  char message[2048];

} cmd_args;


#endif
