#ifndef DELAYED_SEND_H
#define DELAYED_SEND_H
#include <sys/queue.h>
struct message
{
  char destination[1024];
  char data[1024];

    TAILQ_ENTRY (message) entries;
};
typedef struct message message;

void *send_thread (void *arg);
void push_message (char *dest, char *msg);

#endif
