#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>
#include "pong.h"
void *runthread(void *arg){
  pongr *r=arg;
    printf ("$ !%s!", r->command);
fflush(stdout);
    FILE *fp = popen (r->command, "r");
  char buf[256], big_buf[256][256], destination[256], c,line[256];
  int to = 120;
  time_t start = time (NULL), last, now;
  if (fp == NULL)
    {
      printf ("command error\n");
      return;
    }

  int flags, fd = fileno (fp), sent = 0, i, line_count = 0;

  flags = fcntl (fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl (fd, F_SETFL, flags);
 // printf ("run thread started\n");
  last = time (NULL);
  do
    {

      c = fgetc (fp);

      if (isprint(c))
	{
	  ungetc (c, fp);

	  fgets (buf, 255, fp);
	  if (buf[strlen (buf) - 1] == '\n')
	    buf[strlen (buf) - 1] = '\0';
	  // trim (buf);
	  if (strlen (buf) > 0)
	    {
//////////////////////////////
	      snprintf(line,256,"%s %s\n",r->destination,buf);
	            sendto(r->sockfd,line,strlen(line),0,(struct sockaddr *)r->client,sizeof(struct sockaddr_in));
                  printf(">> %s",line);
	      ++line_count;
	    }
	}


    }
  while (time (NULL) < (start + to) && line_count <= 50);
  pclose (fp);
        sendto(r->sockfd,"EOT",3,0,(struct sockaddr *)r->client,sizeof(struct sockaddr_in));

free(arg);
}
int main(int argc,char **argv){
    int sockfd,n,len;
   struct sockaddr_in servaddr,cliaddr;
   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   char buf[1024];
   char *tmp; 
   char reply[256];
   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(8989);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));


   while (1)
   {
    
      
      n = recvfrom(sockfd,buf,1024,0,(struct sockaddr *)&cliaddr,&len);
      	 buf[n]=0;

      if(n<4)printf("Got nothing :(\n");
      else if(strncmp("PING",buf,4)==0){
	printf(">> PONG\n");fflush(stdout);
	sendto(sockfd,"PONG",4,0,
             (struct sockaddr *)&cliaddr,sizeof(cliaddr));
      }
       else if(n>5 && strncmp("RUN",buf,3)==0){
	 pongr *r=malloc(sizeof(pongr));
	 memset(r,0,sizeof(pongr));
	 tmp=strchr(&buf[4],' ');
	 if(tmp!=NULL){
	 snprintf(r->destination,(tmp-(&buf[4]))+1,"%s",&buf[4]);
	 tmp=strchr(tmp,' ');
	 if(tmp!=NULL){
	   tmp[strlen(tmp)-2]='\0';
	 snprintf(r->command,2048,"%s",tmp);
	 r->sockfd=sockfd;
	 r->client=&cliaddr;
	 pthread_t tid;
	  pthread_create (&tid, 0, &runthread, (void *) r);
  pthread_detach (tid);
	}
	 }
       }
      else printf("malformed response %s\n",buf);
     
      
      
   }


return 0;
}