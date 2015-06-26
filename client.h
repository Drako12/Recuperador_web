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
#define MAX_HTTP_STATUS_LEN 32
#define MAX_HTTP_GET_LEN 1024
#define HTTP_PORT "80"
#define FORMAT_S(S) "%" #S "s"
#define FORMAT_D(D) "%" #D "d"
#define RESOLVE_S(S) FORMAT_S(S)
#define RESOLVE_D(D) FORMAT_D(D)

struct cli_req_info 
{
  char uri[MAX_URI_LEN];
  char host[MAX_HOST_LEN];
  char path[PATH_MAX];
  char filename[NAME_MAX];
    
};


#endif

