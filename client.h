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
#define MAX_HOST_LEN 256
#define MAX_URI_LEN 1024
#define MAX_HTTP_STATUS_LEN 64
#define MAX_HTTP_GET_LEN 1024
#define HTTP_PORT "80"
#define MAX_FLAG_LEN 4
#define FORMAT(S)
#define RESOLVE(S) FORMAT(S)

struct cli_req_info 
{
  char uri[MAX_URI_LEN];
  char host[MAX_HOST_LEN];
  char path[PATH_MAX];
  char filename[NAME_MAX];
  char flag[MAX_FLAG_LEN];    

};


#endif

