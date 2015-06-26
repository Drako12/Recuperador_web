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
#include <limits.h>

#define BUFSIZE BUFSIZ
#define HEADERSIZE BUFSIZ
#define MAXHOSTLEN 256
#define MAXPATHLEN 512
#define MAXFILENAMELEN 256
#define MAXURILEN 1024
#define MAXLINELEN 128
#define MAXHTTP_STATUSLEN 32
#define MAXHTTP_GETLEN 1024
#define HTTP_PORT "80"

struct cli_req_info 
{
  char uri[MAXURILEN];
  char host[MAXHOSTLEN];
  char path[MAXPATHLEN];
  char filename[MAXFILENAMELEN];
    
};


#endif

