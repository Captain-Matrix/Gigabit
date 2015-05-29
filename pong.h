#ifndef PONG_H
#define PONG_H
struct pongr{
  int sockfd;
  struct sockaddr_in *client;
  char destination[256];
  char command[2048];
};
typedef struct pongr pongr;
#endif