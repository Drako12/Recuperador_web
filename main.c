#include "network.h"

static int parse_param(int argc, char *url, char *flag, char **uri)
{
  int uri_size = 0;

  if (argc > 3)
  {
    fprintf(stderr, "Bad parameters\n");
    return -1;
  }

  uri_size = strlen(url);
  *uri = (char *) calloc(uri_size + 1, sizeof(char));
  if (*uri == NULL)
  {
    fprintf(stderr, "Memory error: %s\n", strerror(errno));
    return -1;   
  }
       
  /* Removendo http:// da URI */
  if (memcmp(url, "http://", 7) == 0)
    strncpy(*uri, url + 7, uri_size - 6);
  else
    strncpy(*uri, url, uri_size);

  return uri_size;
}

static int get_names(char *uri, int uri_size, char **host, char **path,
                     char **filename)
{
  if (strchr(uri, '/') != NULL)
  {
    *path = strchr(uri, '/') + 1;
    *host = (char *) calloc(uri_size - strlen(*path) + 1, sizeof(char));
    if (*host == NULL)
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

int open_file(FILE **fp, char *filename, char *flag)
{
  char *filep;

  if (strcmp(flag, "-f") == 0)
    filep = "w";
  else
    filep = "wx";
  
  *fp = fopen(filename, filep);
  if (*fp == NULL)
  {
    fprintf(stderr, "File error:%s\n", strerror(errno));
    return -1; 
  }
  return 0;
}

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

char *get_line_or_chr (char **start_line)
{
  char *end_line, *buf_tmp;
  int buf_len;  
  
  end_line = strchr(*start_line, '\n');
  if(end_line != NULL)
  {
    buf_len = end_line - *start_line;  
    buf_tmp = (char*)calloc(buf_len + 1, sizeof(char));
    memcpy(buf_tmp, *start_line, buf_len);
    *start_line = end_line + 1;
    return buf_tmp;
  }
  else
  {
   buf_tmp = (char*)calloc(2, sizeof(char));
   memcpy(buf_tmp, *start_line, 1);
   *start_line = *start_line + 1;
   return buf_tmp;
  }

}
/* 
static int check_http_errors(int sockfds, char **buffer)
{
  char *buf_tmp;
   
  buf_tmp = get_line_or_chr(buffer);
    
  if(buf_tmp[9] != '2')
  {
    fprintf(stderr, "%s", buf_tmp);
    free(buf_tmp);
    return -1;
  }
  free(buf_tmp);
  return 0;
}*/

static int check_header(char **buffer, int sockfds)
{
  int nbuffer_read = 0;
  char *buf_tmp = NULL;      
  char *pbuf_start;   

  nbuffer_read = recv(sockfds, *buffer, BUFSIZE, 0);
  pbuf_start = *buffer;
 
  //if (check_http_errors(sockfds, buffer, header) == -1)
  //  return -1;
  
  buf_tmp = get_line_or_chr(buffer);
  while (memcmp(buf_tmp,"\r", 1) != 0 && memcmp(buf_tmp, "", 1) != 0)
  {
    if (strlen(buf_tmp) == 1)
    {
      nbuffer_read = recv(sockfds, *buffer, BUFSIZE, 0);
      pbuf_start = *buffer;
    }
    buf_tmp =  get_line_or_chr(buffer);
  } 

  free(buf_tmp);
  return nbuffer_read - (*buffer - pbuf_start);
}


/* Essa funcao escreve o buffer recebido dentro dele. */
int get_file(char **buffer, int nbuffer_left, FILE **fp, char *filename,
             int sockfd)
{ 
  int recv_data = 0;
  do
  {
    fwrite(*buffer, 1, recv_data + nbuffer_left, *fp);
    nbuffer_left = 0;
    recv_data = recv(sockfd, *buffer, BUFSIZE, 0);
    if (recv_data < 0)
    {
      fprintf(stderr,"Recv error:%s\n", strerror(errno));
      return -1;
    }        
  } while (recv_data > 0);
  
  fclose(*fp);
  return 0;
}

int main(int argc, char *argv[])
{
  char *buffer, *uri = NULL, *path = NULL, *host = NULL, *filename = NULL;
  int nbuffer_left = 0, sockfdm = -1, uri_size = 0;
  FILE *fp = NULL;
  
  buffer = (char *)calloc(BUFSIZE, sizeof(char));
   
  uri_size = parse_param(argc, argv[1], argv[2], &uri);
  if (uri_size ==  -1)
    goto error;
  
  if (get_names(uri, uri_size, &host, &path, &filename) == -1)
    goto error;
  
  if (open_file(&fp, filename,argv[2]) == -1)
    goto error;

  sockfdm = socket_connect(host);
  if (sockfdm > 0)
    if (send_get(path, host, sockfdm) == -1)
      goto error;
  
  nbuffer_left = check_header(&buffer,sockfdm);
  if (nbuffer_left == -1)
    goto error;

  if (get_file(&buffer, nbuffer_left, &fp, filename, sockfdm));
  
  free(uri);
  free(host);
  close(sockfdm);
  return 0;
      
  error:
    if (sockfdm)
      close(sockfdm);
    if (fp)
      if (remove(filename) != 0)
        fprintf(stderr,"Unable to delete file:%s\n", strerror(errno));
    free(uri);
    free(host);
    return -1;
}


