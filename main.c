#include "network.h"

int socket_connect (char *host)
{
  int sockfd = -1, rv;
  struct addrinfo hints, *servinfo, *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(host, "http", &hints, &servinfo)) != 0)
  {
    fprintf (stderr, "Getaddrinfo error: %s\n", gai_strerror (rv) );
    perror("Error");
    freeaddrinfo(servinfo);
    return -1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      fprintf(stderr, "Socket error: %s\n", strerror(errno));
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      fprintf(stderr, "Connnect error: %s\n", strerror(errno));
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);
  return sockfd;
}

int content (int *isheader, int *isline, char *start_line, int *rbuffer,
    char *buffer)
{
  *isheader = 1;
  *isline = 2;
  if (start_line[0] != '\n')
    start_line += 1;
  start_line += 1;
  return *rbuffer - (start_line - buffer);
}
/* Essa funcao checa pelas mensagems de erro do status http recebido apos o
 * request
 */

int check_http_errors(int sockfds)
{
  int rbuffer;
  char http_msg[1], buf_aux[10];

  rbuffer = recv(sockfds, buf_aux, 10, 0);
  if (rbuffer < 1)
  {
    perror ("ERRO");
    exit (1);
  }
  /* Grava o primeiro digito da mensagem de status http */
  http_msg[0] = buf_aux[9];
  switch (http_msg[0])
  {
    case '2':
      break;
  
    case '3':
      printf("3");
      while(http_msg[0] != '\n')
      {
        recv(sockfds, http_msg, 1 , 0);
        printf("%c", http_msg[0]);
      }     
      exit(1);
 
    case '4':   
      printf("4");
      while(http_msg[0] != '\n')
      {
        recv(sockfds, http_msg, 1 , 0);
        printf("%c", http_msg[0]);
      }
      //if(buf_aux[11] == '0')
      // printf("Error 400, Bad Request\n");
      exit(1);

    case '5':
      printf("5");
      while(http_msg[0] != '\n')
      {
        recv(sockfds, http_msg, 1 , 0);
        printf("%c", http_msg[0]);
      }
      exit(1);

    default:
      printf("Header error\n");
      exit(1);
  }
  return 0;
}

char *readline (char *start_line)
{
  char *end_line;

  end_line = strchr(start_line, '\n');
  return line;

  while(buffer[i] != '\n')
  {
   x =  buffer[i]
  }
}
 
int find_header_end(char *buffer, int sockfds)
{
  int nbuffer_read, isline = 0;
  char *end_line, *start_line;
  start_line = buffer;  
    while(1)
    {
      nbuffer_read = recv(sockfds, buffer, BUFSIZE, 0);
      readline(buffer);
      if(nbuffer_read == 0)
      {
        fprintf(stderr,"Recv error:%s\n", strerror(errno));
        break;
      }
      while(end_line != NULL)
      {
        end_line = readline(start_line, &isline);
        start_line = end_line + 1;
        if(memcmp(start_line, "\n", 1) == 0)
          return strlen(start_line) - 1;
        if (memcmp(start_line, "\r\n", 2) == 0 )        
          return strlen(start_line) - 2; 
      }
    }  
return -1;
}
/* Funcao para formatar a mensagem GET e enviar para o servidor */
int send_get(char *path, char *host, int sockfd)
{
  char get_url[1024];

  if (snprintf(get_url, sizeof(get_url),
        "GET %s HTTP/1.0\r\nHOST:%s\r\n\r\n", path, host) < 0)
  {
    fprintf(stderr, "URL maior que o buffer\n");
    return -1;
  }

  if (send(sockfd, get_url, strlen(get_url), 0) < 0)
  {
    fprintf(stderr, "Send error:%s\n", strerror(errno));
    return -1;
  }
return 0;  
}

/* Essa funcao escreve o buffer recebido dentro dele. */
int get_file(char *path, char *host, FILE *fp, char *filename, int sockfd)
{
  int recv_data, buf_content_size;
  char *buf_content, buffer[BUFSIZE];
  
  memset(buffer, 0, sizeof (buffer) );
  //buf_content_size = skip_header(sockfd, &buffer);

  do{
    fwrite(buffer, 1, recv_data, fp);
    recv_data = recv(sockfd, buffer, BUFSIZE, 0);
    if (recv_data == -1)
    {
      fprintf(stderr, "Recv error: %s\n", strerror(errno));
      return -1;
    }
  }while(recv_data > 0);
  
  fclose(fp);
  return 0;
}

static int parse_param(int argc, char *url, char *flag, char **uri)
{
  int uri_size = 0;

  if(argc > 3)
  {
    fprintf(stderr, "Bad parameters\n");
    return -1;
  }

  uri_size = strlen(url);
  *uri = (char *)calloc(uri_size + 1, sizeof(char));
  if(*uri == NULL)
  {
    fprintf(stderr, "Memory error: %s\n", strerror(errno));
    return -1;   
  }
       
  /* Removendo http:// da URI */
  if(memcmp(url, "http://", 7) == 0)
    strncpy(*uri, url + 7, uri_size - 6);
  else
    strncpy(*uri, url, uri_size);

  return uri_size;
}

static int get_names(char *uri, int uri_size, char **host, char **path, char **filename)
{
  if (strchr(uri, '/') != NULL)
  {
    *path = strchr(uri, '/') + 1;
    *host = (char *)calloc(uri_size - strlen(*path) + 1, sizeof(char));
    if(*host == NULL)
    {
      fprintf(stderr, "Memory error: %s\n", strerror(errno));
      return -1; 
    }
    memcpy(*host, uri, uri_size - strlen(*path) - 1);     
    *filename = strrchr(uri, '/') + 1;
  }
  else
  {
    fprintf(stderr, "Bad URL\n");
    return -1;
  }
  return 0;
}

int open_file(FILE *fp, char *filename, char *flag)
{
  char *filep;

  if(strcmp(flag, "-f") == 0)
    filep = "w";
  else
    filep = "wx";
  
  fp = fopen(filename, filep);
  if(fp == NULL)
  {
    fprintf(stderr, "File error:%s\n", strerror(errno));
    return -1; 
  }
return 0;
}

int main (int argc, char *argv[])
{
  char buffer[BUFSIZE], *uri = NULL, *path = NULL, *host = NULL, *filename = NULL;
  int nbuffer_read = 0, sockfdm = -1, uri_size = 0;
  FILE *fp = NULL;
  
  memset(buffer, 0, sizeof(buffer));
  
  uri_size = parse_param(argc, argv[1], argv[2], &uri);
  if(uri_size ==  -1)
    goto error;
  
  if(get_names(uri, uri_size, &host, &path, &filename) == -1)
    goto error;
  
  if(open_file(fp, filename,argv[2]) == -1)
    goto error;

  sockfdm = socket_connect(host);
  if (sockfdm > 0)
    if(send_get(path, host, sockfdm) == -1)
      goto error;
  
  if(check_http_errors(sockfdm) == -1)
    goto error;
  
  nbuffer_read = find_header_end(&buffer,sockfdm);
  if(get_file(path, host, fp, filename, sockfdm));
  
  free(uri);
  free(host);
  close(sockfdm);
  return 0;
      
  error:
    if(sockfdm)
      close(sockfdm);
    if(fp)
      if(remove(filename) != 0)
        fprintf(stderr,"Unable to delete file:%s\n", strerror(errno));
    free(uri);
    free(host);
    return -1;
}


