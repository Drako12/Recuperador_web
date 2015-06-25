#include "client.h"

/* Realiza um parse dos parametros recebidos no terminal e remove o
 * http:// se estiver presente
 */

static int parse_param(int argc, const char *url, struct name *names)
{
  char *url_s = NULL;
  if (argc > 3)
  {
    fprintf(stderr, "Bad parameters\n");
    return -1;
  }
  
  url_s = strdup(url);
          
  /* Removendo http:// da URI */
  if (memcmp(url_s, "http://", 7) == 0)
    strncpy(names->uri, url_s + 7, MAXURI);
  else
    strncpy(names->uri, url_s, MAXURI);
  
  free(url_s);
  return 0;
}

/* Utiliza a uri completa para copiar o path, host e filename para uma
 * struct
 */

static int get_names(struct name *names)
{
  if (strchr(names->uri, '/') != NULL)
  {
    memcpy(names->path, strchr(names->uri, '/'), MAXPATH);
    memccpy(names->host, names->uri, '/', MAXHOST);
    names->host[strlen(names->host) - 1] = '\0';
    memcpy(names->filename, strrchr(names->uri, '/') + 1, MAXFILE);
  }
  else
  {
    fprintf(stderr, "Bad URL\n");
    return -1;
  }
  return 0;
}

/* Cria o arquivo de acordo com o filename e flag */

FILE *open_file(struct name names, char *flag)
{
  char *filep;

  if (flag)
  { 
    if (strcmp(flag, "-f") == 0)
    filep = "w";
  }
  else
    filep = "wx";
 
  return fopen(names.filename, filep);
}

/* Funcao para abrir o socket e realizar a conexao */

int socket_connect (struct name names)
{
  int sockfd = -1, rv;
  struct addrinfo hints, *servinfo, *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(names.host, "http", &hints, &servinfo)) != 0)
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

int send_get(struct name names, int sockfd)
{
  char get_url[1024];

  if (snprintf(get_url, sizeof(get_url),
        "GET %s HTTP/1.0\r\nHOST:%s\r\n\r\n", names.path, names.host) < 0)
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

/* Essa funcao le uma linha de um buffer ate encontrar uma nova linha,
 * escreve ela em um buffer temporario,  e retorna o tamanho dela
 */

int get_line(char *buffer, char *buf_tmp)
{
  char *end_line, *pbuffer;
  int buf_len;
  
  pbuffer = buffer;
  end_line = strchr(buffer, '\n');
  if (end_line != NULL)
  {
    buf_len = end_line - pbuffer + 1;
    memcpy(buf_tmp, buffer, buf_len);
 //   pbuffer = end_line + 1;
    return buf_len;
  }
  
  fprintf(stderr,"Header error");
  return -1;    
  
}

/* Checa por erros http */

static int check_http_errors(char *header)
{
  char buf_tmp[MAXLINE];
  char http_status[MAXHTTP_STATUS];
  int  status = 0, ns = 0;
   
  if(get_line(header, buf_tmp) != -1)   
  {    
    ns = sscanf(buf_tmp,"%*[^ ]%d %[aA-zZ ]", &status, http_status);   
    if (ns != 2)
    {
      fprintf(stderr, "Header error");
      return -1;
    }
    else
    {
      if (status < 200 && status > 299)
      {
        fprintf(stderr,"Header:%s",http_status);
        return -1;
      }
    }
  }
  else
    return -1;
  return 0;
}

/* Funcao para encontrar o final do header, ele e lido linha a linha ate
 * encontrar uma linha que tem somente uma nova linha (ou carrier seguido
 * de linha)
 */

int check_header_end(char *header)
{
  int nheader_read = 0;
  char buf_tmp[BUFSIZE];      
  char *pheader;   
  
  memset(buf_tmp, 0, BUFSIZE);
  pheader = header;
  
  while (memcmp(buf_tmp,"\r\n", 2) != 0 && memcmp(buf_tmp, "\n", 1) != 0)
  {
    memset(buf_tmp, 0, BUFSIZE);
    nheader_read = get_line(pheader + nheader_read, buf_tmp) +  nheader_read;
    if(nheader_read == -1)
      return -1;                              
  }
  return nheader_read;
}

/* Copia todo o header e mais um pedaco do conteudo para uma variavel e 
 * retorna o espaco restante (nao utilizado) da variavel
 */

static int get_header(char *header, int sockfds)
{
  int nheader_read = 0;
  int buf_left = HEADERSIZE;
  char *pheader;

  pheader = header;
  while(nheader_read < buf_left)
  {   
    nheader_read = recv(sockfds, pheader, BUFSIZE, 0);
    if(nheader_read == 0)
      break;
    buf_left -= nheader_read;
    pheader += nheader_read;  
  }
  return buf_left;  
}

/* Essa funcao escreve o buffer recebido dentro do arquivo aberto */

int get_file(char *buffer, char *header, int buf_left,  int header_pos,
             FILE *fp, struct name names, int sockfds)
{ 
  int recv_data = 0;
  fwrite(header + header_pos, 1, HEADERSIZE - buf_left - header_pos, fp);
  do
  {    
    recv_data = recv(sockfds, buffer, BUFSIZE, 0);
    if (recv_data < 0)
    {
      fprintf(stderr,"Recv error:%s\n", strerror(errno));
      return -1;
    }        
    fwrite(buffer, 1, recv_data, fp);
  }while(recv_data > 0); 
  
  fclose(fp);
  return 0;
}

int main(int argc, char *argv[])
{
  char buffer[BUFSIZE], header[HEADERSIZE];
  int buf_left = 0, header_pos = 0, sockfdm = -1;
  FILE *fp = NULL;
  struct name names;
   
  if (parse_param(argc, argv[1], &names) == -1)
    goto error;
  
  if (get_names(&names) == -1)
    goto error;
  
  if (!(fp = open_file(names, argv[2])))
  {
     fprintf(stderr, "File error:%s\n", strerror(errno));
     goto error;
  }  

  sockfdm = socket_connect(names);
  if (sockfdm > 0)
    if (send_get(names, sockfdm) == -1)
      goto error;
  
  
  if ((buf_left = get_header(header, sockfdm)) == -1)
    goto error;
  
  if (check_http_errors(header) == -1)
    goto error;
  
  if ((header_pos = check_header_end(header)) == -1)
    goto error;
  
  if (get_file(buffer, header, buf_left, header_pos, fp, names, sockfdm) == -1)
    goto error;
  
  close(sockfdm);
  return 0;
      
error:
    if (sockfdm)
      close(sockfdm);
    if (fp)
      if (remove(names.filename) != 0)
        fprintf(stderr,"Unable to delete file:%s\n", strerror(errno));
return -1;
}


