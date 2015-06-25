/*!
 * \file main.c
 * \brief Cliente para download de arquivos 
 * \date 25/06/2015
 * \author Diogo Morgado <diogo.teixeira@aker.com.br>
 */

#include "client.h"

/*!
 * \brief Realiza um parse dos parametros recebidos no terminal e remove o
 *        http:// se estiver presente.
 * \param[in] argc Numero de parametros do comando
 * \param[in] url Segundo parametro do comando
 * \param[out] http_names Estrutura que ira receber a URL completa
 * \return 0 se for OK 
 * \return -1 se der algum erro
 */

static int parse_param(int argc, const char *url,
                       struct http_addr_names *http_names)
{
  char *url_s = NULL;

  if (argc > 3 || argc == 0)
  {
    fprintf(stderr,"%s\n%s\n%s", "Bad parameter", "Usage: dget [URL] [OPTION]",
            "-f, force overwrite");
    return -1;
  }
  
  url_s = strdup(url);
  
  if(strlen(url_s) > MAXURILEN)
  {
    fprintf(stderr, "Bad URL\n");
    return -1;
  }
    
  if (strncmp(url_s, "http://", 7) == 0)
    strncpy(http_names->uri, url_s + 7, MAXURILEN);
  else
    strncpy(http_names->uri, url_s, MAXURILEN);
  
  free(url_s);
  
  return 0;
}

/*!
 * \brief Utiliza a uri completa para copiar o path, host e filename para 
 *        http_names
 * \param[out] http_names Estrutura que ira receber os nomes do host, path e
 *             filename
 * \return 0 se for OK
 * \return -1 se der erro
 */

static int get_names(struct http_addr_names *http_names)
{
  if (sscanf(http_names->uri,"%[^/]%s", http_names->host,
            http_names->path) != 2)
  {
    fprintf(stderr, "Bad URL:%s\n", strerror(errno));
    return -1;
  }
  else
    strncpy(http_names->filename, strrchr(http_names->path, '/') + 1,
           MAXFILENAMELEN);
 
  return 0;
}

/*!
 * \brief Cria o arquivo de acordo com o filename e flag
 * \param[in] http_names Estrutura com o nome do arquivo
 * \param[in] flag Opcao para forcar a sobreescrita do arquivo
 * \return Ponteiro do arquivo aberto
 */

static FILE *open_file(struct http_addr_names *http_names, char *flag)
{
  char *filep;

  if (flag)
  { 
    if (strcmp(flag, "-f") == 0)
    filep = "w";
  }
  else
    filep = "wx";
 
  return fopen(http_names->filename, filep);
}

/*!
 * \brief Cria o socket e conecta ele ao endereco especificado
 * \param[in] http_names Estrutura com o nome do host
 * \return Descritor do socket
 */

static int socket_connect (const struct http_addr_names *http_names)
{
  int sockfd = -1, ret = 0;
  struct addrinfo hints, *servinfo, *aux;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((ret = getaddrinfo(http_names->host, "http", &hints, &servinfo)) != 0)
  {
    fprintf (stderr, "Getaddrinfo error: %s\n", gai_strerror (ret) );
    return -1;
  }

  for (aux = servinfo; aux != NULL; aux = aux->ai_next)
  {
    if ((sockfd = socket(aux->ai_family, aux->ai_socktype,
         aux->ai_protocol)) == -1)
    {
      fprintf(stderr, "Socket error: %s\n", strerror(errno));
      continue;
    }

    if (connect(sockfd, aux->ai_addr, aux->ai_addrlen) == -1)
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

/*!
 * \brief Formata a mensagem HTTP_GET e envia para o servidor
 * \param[in] http_names Estrutura com o nome do host e path para o arquivo
 * \param[in] sockfd Descritor do socket
 * \return 0 se for ok
 * \return -1 se der algum erro
 */ 

static int send_http_get(const struct http_addr_names *http_names, int sockfd)
{
  char http_get_msg[MAXHTTP_GETLEN];

  if (snprintf(http_get_msg, sizeof(http_get_msg),
      "GET %s HTTP/1.0\r\nHOST:%s\r\n\r\n", http_names->path,
      http_names->host) < 0)
  {
    fprintf(stderr, "Error building http get message:%s\n", strerror(errno));
//    fprintf(stderr, "URL maior que o buffer\n");
    return -1;
  }

  if (send(sockfd, http_get_msg, strlen(http_get_msg), 0) < 0)
  {
    fprintf(stderr, "Send error:%s\n", strerror(errno));
    return -1;
  }
return 0;  
}

/*!
 * \brief Copia todo o header e mais um pedaco do conteudo para uma variavel  
 * \param[in] sockfd Descritor do socket
 * \param[out] header Variavel que contem o header da resposta do HTTP GET
 * \return 0 se for OK
 * \return -1 se der algum erro
 */


static int get_http_header(char *header, int sockfd)
{
  int num_bytes_header = 0;
  int num_bytes_aux = 0;
  char *header_aux;

  header_aux = header;
  while (num_bytes_aux < HEADERSIZE - 1)
  {   
    num_bytes_header = recv(sockfd, header_aux, HEADERSIZE - num_bytes_aux -
                            1, 0);
    if (num_bytes_header < 0)
    {
      fprintf(stderr, "Recv error:%s\n", strerror(errno));
      return -1;
    }
    if (num_bytes_header == 0)
      break;

    num_bytes_aux += num_bytes_header;
    header_aux += num_bytes_header;  
  }

  *header_aux = 0;  

  return 0;  
}

/*!
 * \brief Essa funcao le uma linha de um buffer ate encontrar uma nova linha,
 *        escreve ela em um buffer temporario,  e retorna o tamanho dela
 * \param[in] buffer Ponteiro para uma posicao de um buffer
 * \param[out] buf_tmp Ponteiro para um buffer temporario
 * \return buf_len Retorna tamanho do buffer temporario
 * \return -1 se der algum erro
 */

static int get_line(char *buffer, char *buf_tmp)
{
  char *end_line, *buffer_start;
  int buf_len = 0;
  
  buffer_start = buffer;
  end_line = strchr(buffer, '\n');
  if (end_line != NULL)
  {
    buf_len = end_line - buffer_start + 1;
    strncpy(buf_tmp, buffer, buf_len);
    return buf_len;
  }
  
  fprintf(stderr,"Header error");
  
  return -1;    
}

/*! 
 * \brief Checa por erros http
 * \param[in] header Ponteiro para o inicio do header
 * \return 0 se for OK
 * \return -1 se der algum erro
 */

static int check_http_errors(char *header)
{
  char buf_tmp[MAXLINELEN];
  char http_status[MAXHTTP_STATUSLEN];
  int  status = 0, ret = 0;
  
  memset(buf_tmp, 0, sizeof(buf_tmp));

  if (get_line(header, buf_tmp) != -1)   
  {    
    ret = sscanf(buf_tmp,"%*[^ ]%d %[aA-zZ ]", &status, http_status);   
    if (ret != 2)
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

/*!
 * \brief Funcao para encontrar o final do header, ele e lido linha a linha ate
 * encontrar uma linha que tem somente uma nova linha (ou carrier seguido
 * de linha)
 * \param[in] header Variavel que contem o header
 * \return 0 se for OK
 * \return 1 se der algum erro
 */

static int check_header_end(char *header)
{
  int num_bytes_header = 0;
  char buf_tmp[HEADERSIZE];      
  char *header_aux;   
  
  memset(buf_tmp, 0, HEADERSIZE);
  header_aux = header;
  
  while (memcmp(buf_tmp,"\r\n", 2) != 0 && memcmp(buf_tmp, "\n", 1) != 0)
  {
    memset(buf_tmp, 0, HEADERSIZE);
    num_bytes_header = get_line(header_aux + num_bytes_header, buf_tmp) +
                                num_bytes_header;
    if (num_bytes_header == -1)
      return -1;                              
  }
  
  return num_bytes_header;
}

/*! 
 * \brief Essa funcao escreve o buffer recebido dentro do arquivo aberto
 * \param[in] header Variavel que contem o header e um pedaco do conteudo 
 * \param[in] header_len Variavel que contem o tamanho do header 
 * \param[in] sockfd Descritor do socket
 * \param[out] buffer Variavel para qual os dados recebidos serao enviados
 * \param[out] fp Descritor do arquivo aberto 
 * \return 0 se for OK
 * \return -1 se der algum erro
 */ 

static int get_file(char *buffer, char *header, int header_len,
                    FILE *fp, int sockfd)
{ 
  int recv_data = 0;
  fwrite(header + header_len, 1, HEADERSIZE - header_len - 1, fp);
  do
  {    
    recv_data = recv(sockfd, buffer, BUFSIZE, 0);
    if (recv_data < 0)
    {
      fprintf(stderr,"Recv error:%s\n", strerror(errno));
      return -1;
    }        
    fwrite(buffer, 1, recv_data, fp);
  } while(recv_data > 0); 
  
  fclose(fp);
  return 0;
}
/*! 
 * \brief Funcao principal que ira realizar as chamadas das funcoes do programa
 * \param[in] argc Numero de parametros recebidos
 * \param[in] argv Vetor com os parametros separados em strings
 * \return 0 se for OK
 * \return -1 se der algum erro
 */

int main(int argc, char *argv[])
{
  char buffer[BUFSIZE], header[HEADERSIZE];
  int  header_len = 0, sockfd = -1;
  FILE *fp = NULL;
  struct http_addr_names http_names;
   
  if (parse_param(argc, argv[1], &http_names) == -1)
    goto error;
  
  if (get_names(&http_names) == -1)
    goto error;
  
  if (!(fp = open_file(&http_names, argv[2])))
  {
     fprintf(stderr, "File error:%s\n", strerror(errno));
     goto error;
  }  

  sockfd = socket_connect(&http_names);
  if (sockfd > 0)
  {  if (send_http_get(&http_names, sockfd) == -1)
       goto error;
  }
  else
    goto error;
  
  if (get_http_header(header, sockfd) == -1)
    goto error;
  
  if (check_http_errors(header) == -1)
    goto error;
  
  if ((header_len = check_header_end(header)) == -1)
    goto error;
  
  if (get_file(buffer, header, header_len, fp, sockfd) == -1)
    goto error;
  
  close(sockfd);
  return 0;
      
error:
    if (sockfd)
      close(sockfd);
    if (fp)
      if (remove(http_names.filename) != 0)
        fprintf(stderr,"Unable to delete file:%s\n", strerror(errno));
return -1;
}


