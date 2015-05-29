#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "run.h"
#include "Gigabit.h"
#include "delayed_send.h"
void *
ping(void *arg){
  system("/crypt/sb/startpong.sh");  
   sleep(120);

  int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   
   char recvping[5];

   sockfd=socket(AF_INET,SOCK_DGRAM,0);
int flags = fcntl(sockfd, F_GETFL);
flags |= O_NONBLOCK;
fcntl(sockfd, F_SETFL, flags);
   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr("192.168.2.199");
   servaddr.sin_port=htons(8990);

   while (1)
   {
      sendto(sockfd,"PING",4,0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
      sleep(5);
      
      n=recvfrom(sockfd,recvping,5,0,NULL,NULL);
      if(n<4){
	printf("Got nothing :(\n");
  //system("/crypt/sb/startpong.sh");  
  //sleep(120);
      }
      else if(strncmp("PONG",recvping,4)==0)
	printf("PONG!\n");
      else printf("Ping> malformed response %s\n",recvping);
      recvping[5]=0;
      
      
   } 
  
}
void *recycle(void *arg){
  irc_session_t *session=arg;
      context_t *context = (context_t *) irc_get_ctx (session);

  context->runnerup=0;
 reload(1800);
     context->runnerup=1;

}
void reload(int wait){
   //system("/crypt/sb/startpong.sh");  
   // sleep(wait);
}
void *
run (void *arg)
{

  runtime *rt = (runtime *) arg;
    context_t *context = (context_t *) irc_get_ctx (rt->session);

//context_t *context=(context_t*) irc_get_ctx(rt->session);
if(!context->runnerup){
 push_message(rt->reply,"Reloading,please wait..."); 
}
  char buf[256], destination[256],*tmp;
  int to = rt->timeout;
  //printf ("##%s##%s##%s##\n", destination, rt->reply, rt->command);
 snprintf(buf,2048,"RUN %s %s\r\n\0",rt->reply,rt->command);
 
///////////////////////////////////////////////////////////////////////////////////////
     int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   
   char recvping[5];

   sockfd=socket(AF_INET,SOCK_DGRAM,0);
int flags = fcntl(sockfd, F_GETFL);
flags |= O_NONBLOCK;
fcntl(sockfd, F_SETFL, flags);
   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr("192.168.2.199");
   servaddr.sin_port=htons(8989);
  printf ("run thread started\n");  
 ///////////////////////////////////////////////////////////////////////////////////////
 sendto(sockfd,buf,strlen(buf),0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
 while(1){
        n=recvfrom(sockfd,buf,256,0,NULL,NULL);
	if(n>0){
        buf[n]='\0';
        if(strncmp("EOT",buf,3)==0)break;
	else {
	  tmp=strchr(&buf[0],' ');
	  if(tmp==NULL)break;
	  else{
	    snprintf(destination,(tmp-&buf[0])+1,"%s",buf);
	    printf("<<%s>>%s\n",destination,tmp);
	    push_message(destination,tmp);
	    
	  }
	}
	}
 }
 
 
  free (rt);

  return;
}

void
rt_run (int t, char *to, char *channel, char *start_info, char *program,
	char *args, irc_session_t * s)
{
  char tmp[256];
  pthread_t tid = (pthread_t) g_rand ();
  runtime *rt = malloc (sizeof (runtime));
  rt->timeout = t;

  snprintf (rt->reply, 256, "%s", to);
  rt->session = s;
  if (start_info)
    {
      snprintf (tmp, 256,
		"%s,Output will be messaged to %s", start_info, rt->reply);
     // push_message (channel, tmp);
    }
  if (args != NULL)
    snprintf (rt->command, 1024, "timeout -sSIGKILL %d %s %s", rt->timeout,
	      program, args);
  else
    snprintf (rt->command, 1024, "timeout -sSIGKILL %d %s", rt->timeout,
	      program);

  printf ("running \"%s\"\n", rt->command);
  pthread_create (&tid, 0, &run, (void *) rt);
  pthread_detach (tid);
  // pthread_join (tid, NULL);

}

/*
int
main (int argc, char **argv)
{
  pthread_t tid;
  runtime *rt = malloc (sizeof (runtime));
  rt->timeout = 10;
  rt->command = malloc (1024);
  sprintf (rt->command, "timeout -sSIGKILL %d find ~", rt->timeout);

  pthread_create (&tid, 0, &run, (void *) rt);
  pthread_join (tid, NULL);

  free (rt->command);
  free (rt);
  return 0;
}*/
