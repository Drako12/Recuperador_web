/*!
 * \brief Cliente para download de arquivos 
 * \date 25/06/2015
 * \author Diogo Morgado <diogo.teixeira@aker.com.br>
 */

#include "client.h"

/*!
 * \brief Realiza um parse dos parametros recebidos no terminal e remove o
 *        http:// se estiver presente.
 *
 * \param[in] argc Numero de parametros do comando
 * \param[in] url Segundo parametro do comando
 * 
 * \param[out] req_info Estrutura que ira receber a URL completa
 * 
 * \return 0 se for OK 
 * \return -1 se der algum erro
 */

static int parse_param(int n_params, const char *url,
                       struct cli_req_info *req_info)
{
  char *url_s = NULL;

  if (n_params > 3 || n_params == 0)
  {
    fprintf(stderr, "Bad parameters\nUsage: dget [URL] [OPTION]\n"
            "-f, force overwrite");
    return -1;
  }
  
  url_s = strdup(url);
  if (url_s == NULL)
  {
    fprintf(stderr, "Error allocating memory:\n%s", strerror(errno));
    return -1;    
  }

  if(strlen(url_s) > MAX_URI_LEN)
  {
    fprintf(stderr, "Bad URL\n");
    return -1;
  }
    
  if (strncmp(url_s, "http://", 7) == 0)
    strncpy(req_info->uri, url_s + 7, MAX_URI_LEN);
  else
    strncpy(req_info->uri, url_s, MAX_URI_LEN);
  
  free(url_s);
  
  return 0;
}

/*!
 * \brief Utiliza a uri completa para copiar o path, host e filename para 
 *        req_info
 *
 * \param[out] req_info Estrutura que ira receber os nomes do host, path e
 *             filename
 * 
 * \return 0 se for OK
 * \return -1 se der erro
 */

static int get_names(struct cli_req_info *req_info)
{
  if (sscanf(req_info->uri,"%[^/]%s", req_info->host, req_info->path) != 2)
  {
    fprintf(stderr, "Bad URL:%s\n", strerror(errno));
    return -1;
  }
  else
    strncpy(req_info->filename, strrchr(req_info->path, '/') + 1, NAME_MAX);
 
  return 0;
}

/*!
 * \brief Cria o arquivo de acordo com o filename e flag
 *
 * \param[in] req_info Estrutura com o nome do arquivo
 * \param[in] flag Opcao para forcar a sobreescrita do arquivo
 * 
 * \return Ponteiro do arquivo aberto
 */

static FILE *open_file(struct cli_req_info *req_info, char *flag)
{
  char *filep;

  if (flag)
  { 
    if (strcmp(flag, "-f") == 0)
    filep = "w";
    else
    {
      fprintf(stderr,"Bad parameter\n Usage: dget [URL] [OPTION]\n"
              "-f, force overwrite");
      return NULL;
    }
  }
  else
    filep = "wx";
 
  return fopen(req_info->filename, filep);
}

/*!
 * \brief Cria o socket e conecta ele ao endereco especificado
 *
 * \param[in] req_info Estrutura com o nome do host
 * 
 * \return Descritor do socket
 */

static int socket_connect(const char *host)
{
  int sockfd = -1, ret = 0;
  struct addrinfo hints, *servinfo, *aux;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((ret = getaddrinfo(host, HTTP_PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "Getaddrinfo error: %s\n", gai_strerror(ret));
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
 *
 * \param[in] req_info Estrutura com o nome do host e path para o arquivo
 * \param[in] sockfd Descritor do socket
 * 
 * \return 0 se for ok
 * \return -1 se der algum erro
 */ 

static int send_http_get(const struct cli_req_info *req_info, int sockfd)
{
  char http_get_msg[MAX_HTTP_GET_LEN];

  if (snprintf(http_get_msg, sizeof(http_get_msg),
      "GET %s HTTP/1.0\r\nHOST:%s\r\n\r\n", req_info->path,
      req_info->host) < 0)
  {
    fprintf(stderr, "Error building http get message:%s\n", strerror(errno));
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
 * 
 * \param[in] sockfd Descritor do socket
 * 
 * \param[out] header Variavel que contem o header da resposta do HTTP GET
 * 
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
 * 
 * \param[in] buffer Ponteiro para uma posicao de um buffer
 * 
 * \param[out] buf_tmp Ponteiro para um buffer temporario
 * 
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
    buf_tmp[buf_len] = '\0';
    return buf_len;
  }
  
  fprintf(stderr,"Header error");
  
  return -1;    
}

/*! 
 * \brief Checa por erros http
 * 
 * \param[in] header Ponteiro para o inicio do header
 * 
 * \return 0 se for OK
 * \return -1 se der algum erro
 */

static int check_http_errors(char *header)
{
  char buf_tmp[LINE_MAX];
  char http_status[MAX_HTTP_STATUS_LEN];
  int  status = 0, ret = 0;
  
  memset(buf_tmp, 0, sizeof(buf_tmp));

  if (get_line(header, buf_tmp) != -1)   
  {   
   //ret = sscanf(buf_tmp, "%*[^ ]"RESOLVE_D(LINE_MAX)" "RESOLVE_S(MAX_HTTP_STATUS_LEN), &status, http_status);
   ret = sscanf(buf_tmp, "%*[^ ] %d %s", &status, http_status);   
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
 * 
 * \param[in] header Variavel que contem o header
 *
 * \return num_bytes_header se for OK
 * \return 1 se der algum erro
 */

static int check_header_end(char *header)
{
  int num_bytes_header = 0;
  //char buf_tmp[HEADERSIZE];      
  char *header_aux;   
  
 // memset(buf_tmp, 0, HEADERSIZE);
 // header_aux = header;
  
 // while (memcmp(buf_tmp,"\r\n", 2) != 0 && memcmp(buf_tmp, "\n", 1) != 0)
 // {
 //   num_bytes_header = get_line(header_aux + num_bytes_header, buf_tmp) +
 //                               num_bytes_header;
 //   if (num_bytes_header == -1)
 //     return -1;                              
 // }
  
  if ((header_aux =  strstr(header,"\r\n\r\n")) != NULL)
  {
    num_bytes_header = header_aux - header + 4;
    return num_bytes_header;
  }
  
  if ((header_aux =  strstr(header,"\n\n")) != NULL)
  {
    num_bytes_header = header_aux - header + 2;
    return num_bytes_header;
  } 

  return -1;
   
  //return num_bytes_header;
}

/*! 
 * \brief Essa funcao escreve o buffer recebido dentro do arquivo aberto
 * 
 * \param[in] header Variavel que contem o header e um pedaco do conteudo 
 * \param[in] header_len Variavel que contem o tamanho do header 
 * \param[in] sockfd Descritor do socket
 * \param[out] buffer Variavel para qual os dados recebidos serao enviados
 * \param[out] fp Descritor do arquivo aberto 
 * 
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
      fprintf(stderr, "Recv error:%s\n", strerror(errno));
      return -1;
    }        
    fwrite(buffer, 1, recv_data, fp);
  } while(recv_data > 0); 
  
  fclose(fp);
  return 0;
}

int main(int argc, char *argv[])
{
  char buffer[BUFSIZE], header[HEADERSIZE];
  int  header_len = 0, sockfd = -1;
  FILE *fp = NULL;
  struct cli_req_info req_info;
  
  //req_info = { 0 };
  memset(&req_info, 0, sizeof(req_info));
  memset(buffer, 0, sizeof(buffer));

  if (parse_param(argc, argv[1], &req_info) == -1)
    goto error;
  
  if (get_names(&req_info) == -1)
    goto error;
  
  if (!(fp = open_file(&req_info, argv[2])))
  {    
    fprintf(stderr, "File error:%s\nUsage: dget [URL] [OPTION]\n"
            "-f, force overwrite", strerror(errno));
    goto error;
  }  

  if ((sockfd = socket_connect(req_info.host)) < 0)
    goto error;
  
  if (send_http_get(&req_info, sockfd) == -1)
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
      if (unlink(req_info.filename) != 0)
        fprintf(stderr,"Unable to delete file:%s\n", strerror(errno));
return -1;
}


