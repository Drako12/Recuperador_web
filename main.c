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
      fprintf(stderr, "Socket error: %s\n", strerror(errno);
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      fprintf(stderr, "Connnect error: %s\n", strerror(errno);
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

void check_error(int sockfds)
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
}
  
/*
 * Essa funcao marca o fim do header lendo o buffer até encontrar /r/n/r/n
 * (ou variacoes). O buffer 'e lido e um estado para saber se já foi
 * encontrada uma linha 'e mantido para caso a condicao para fim do header
 * esteja separada entre buffers. O conteudo do buffer alem do header e
 * escrito no arquivo.
 */

FILE *skip_header (int sockfds, char *buffer, char *flag, char *filename)
{
  int rbuffer;
  int content_size = 0, isline = 0, isheader = 0;
  char *end_line, *start_line;
  FILE *fp;
  
  check_error(sockfds);

  if (strncmp(flag, "-f", 2) == 0)
  {
    fp = fopen(filename, "w");
    if (fp == NULL)
    {
      perror("ERRO:");
      exit(1);
    }
  }
  else
  {
    fp = fopen(filename, "wx");
    if (fp == NULL)
    {
      perror("ERRO:");
      exit(1);
    }
  }

  while (isheader == 0)
  {
    rbuffer = recv (sockfds, buffer, BUFSIZE, 0);
    if (rbuffer < 1)
    {
      perror ("ERRO");
      exit (1);
    }
    start_line = buffer;

    /*
     * Essa condição serve para o caso do final do header estar quebrado pelo
     * tamanho do buffer.
     */
    if ((!(memcmp(start_line, "\r\n", 2)) || !(memcmp(start_line, "\n", 1)))
         && isline == 1)
    {
      content_size = content(&isheader, &isline, start_line, &rbuffer, buffer);
      fwrite(buffer + (rbuffer - content_size), 1, content_size, fp);
      isheader = 1;
      return fp;
    }
    else
    {
      isline = 0;
    }
    /*
     * Esse loop procura por novas linhas e desloca o ponteiro para o caractere
     * seguinte, procurando em seguida pelo fim do header (nova linha ou carrier
     * seguido de linha.
     */
    while ((end_line = index(start_line, '\n')) != NULL)
    {
      start_line = end_line + 1;
      isline = 1;
      if ((!(memcmp(start_line, "\r\n", 2))) ||
          (!(memcmp(start_line, "\n", 1))))
      {
        content_size = content(&isheader, &isline, start_line, &rbuffer,
                               buffer);
        fwrite(buffer + (rbuffer - content_size), 1, content_size, fp);
        isheader = 1;
        return fp;
      }
    }
  }
return 0;
}
/* Funcao para formatar a mensagem GET e enviar para o servidor */
void send_get(char *path, char *host, int sockfd)
{
  char get_url[1000];

  if (snprintf(get_url, sizeof(get_url),
        "GET %s HTTP/1.0\r\nHOST:%s\r\n\r\n", path, host) < 0)
  {
    printf("URL maior que o buffer");
    exit(1);
  }

  if (send(sockfd, get_url, strlen(get_url), 0) < 0)
  {
    perror("ERRO NO SEND:");
    exit (1);
  }
}
/* Essa funcao cria o arquivo e escreve o buffer recebido dentro dele. */
int get_file(char *path, char *host, char *filename, char *flag, int sockfd)
{
  int recv_data;
  char buffer[BUFSIZE];
  FILE *fp;

  send_get(path, host, sockfd);
  memset(buffer, 0, sizeof (buffer) );
  fp = skip_header(sockfd, buffer, flag, filename);
  recv_data = recv(sockfd, buffer, BUFSIZE, 0);
  if (recv_data == -1)
  {
    fprintf(stderr, "Recv error: %s\n", strerror(errno);
    return -1;
  }

  while (recv_data > 0)
  {
    fwrite(buffer, 1, recv_data, fp);
    recv_data = recv(sockfd, buffer, BUFSIZE, 0);
    if (recv_data == -1)
    {
      fprintf(stderr, "Recv error: %s\n", strerror(errno);
      return -1;
    }
  }
  fclose(fp);
}

int main (int argc, char *argv[])
{
  char *uri, *path = NULL, *host = NULL, *filename = NULL;
  int sockfdm = -1, uri_size = 0;
  
  if(argc > 3)
  {
    fprintf(stderr, "Bad parameters\n");
    return -1;
  }
  uri_size = strlen(argv[1]);
  uri = (char *)calloc(uri_size + 1, sizeof(char));
  if(uri == NULL)
  {
    fprintf(stderr, "Memory error: %s\n", strerror(errno);
    goto error;   
  }
  strncpy(uri, argv[1], uri_size);
     
  /* Removendo http:// da URI */
  if(memcmp(uri, "http://", 7) == 0)
    strncpy(uri, argv[1] + 7, uri_size - 6);
  else
    strncpy(uri, argv[1], uri_size);

  if (strchr(uri, '/') != NULL)
  {
    path = strchr(uri, '/') + 1;
    host = (char *)calloc(uri_size - strlen(path) + 1, sizeof(char));
    if(host == NULL)
    {
      fprintf(stderr, "Memory error: %s\n", strerror(errno);
      goto error; 
    }
    memcpy(host, uri, uri_size - strlen(path) - 1);     
    filename = strrchr(uri, '/') + 1;
  }
  else
  {
    fprintf(stderr, "Bad URL\n");
    goto error;
  }

  sockfdm = socket_connect(host);

  if (sockfdm > 0)
    get_file(path, host, filename, argv[2], sockfdm);
  
  free(uri);
  free(host);
  close(sockfdm);
  return 0;

  error:    
    free(uri);
    free(host);
    return -1;
 
}


