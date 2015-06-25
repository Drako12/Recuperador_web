#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE BUFSIZ
#define HEADERSIZE BUFSIZ
#define MAXHOST 256
#define MAXPATH 512
#define MAXFILE 256
#define MAXURI 1024
#define MAXLINE 128
#define MAXHTTP_STATUS 32

struct name 
{
  char uri[MAXURI];
  char host[MAXHOST];
  char path[MAXPATH];
  char filename[MAXFILE];
    
};


#endif

