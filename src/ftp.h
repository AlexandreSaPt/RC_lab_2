#ifndef FTP_H
#define FTP_H

#include <stddef.h>

// ---- Connection ----
#define FTP_CODE_SERVICE_READY       220 
#define FTP_CODE_CLOSING_CTRL        221 

// ---- Login ----
#define FTP_CODE_USERNAME_OK         331  
#define FTP_CODE_LOGIN_SUCCESS       230  
#define FTP_CODE_LOGIN_FAIL          530  

// ---- Passive Mode ----
#define FTP_CODE_ENTERING_PASSIVE    227 

// ---- Retrieve ----
#define FTP_CODE_OPENING_DATA        150   
#define FTP_CODE_DATA_ALREADY_OPEN   125   
#define FTP_CODE_TRANSFER_COMPLETE   226  

// ---- Errors gerais ----
#define FTP_CODE_FILE_UNAVAILABLE    550  

/**
 * Lê uma linha da ligação FTP (control connection),
 * terminada em '\n' (o servidor envia "\\r\\n").
 *
 * @param sockfd  descritor do socket TCP (controlo)
 * @param buf     buffer de saída
 * @param max     tamanho máximo do buffer
 * @return número de bytes lidos (>0) ou -1 em erro
 */
int ftp_read_line(int sockfd, char *buf, size_t max);

/**
 * Envia um comando FTP (sem CRLF) e lê uma linha de resposta.
 * Exemplo: cmd = "USER anonymous"
 *
 * @param sockfd    socket de controlo
 * @param cmd       string do comando (sem "\\r\\n")
 * @param response  buffer para a resposta do servidor
 * @param max       tamanho máximo do buffer de resposta
 * @return 0 em sucesso, -1 em erro
 */
int ftp_send_cmd(int sockfd, const char *cmd, char *response, size_t max);

/**
 * Extrai o código numérico (3 dígitos) do início de uma resposta FTP.
 * Exemplo: "220 Service ready" -> 220
 *
 * @param response  string com a resposta completa
 * @return código (>=100) em sucesso, -1 em erro (sem 3 dígitos no início)
 */
int ftp_get_reply_code(const char *response);

/**
 * Faz LOGIN no servidor FTP usando USER / PASS.
 *
 * Fluxo típico:
 *  - ler a primeira linha do servidor (220 ...)
 *  - enviar USER <user>
 *  - se código 331, enviar PASS <pass>
 *  - verificar código 230 (login OK)
 *
 * @param ctrl_sock  socket da ligação de controlo (porta 21)
 * @param user       username (ex: "anonymous")
 * @param password   password (ex: "anonymous@")
 * @return 0 em sucesso, -1 em erro (login falhou)
 */
int ftp_login(int ctrl_sock, const char *user, const char *password);

/**
 * Entra em modo PASSIVE (PASV) e obtém IP e porta
 * para a ligação de dados.
 *
 * Fluxo:
 *  - enviar "PASV"
 *  - ler resposta "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
 *  - construir string IP "h1.h2.h3.h4"
 *  - calcular porto p1*256 + p2
 *
 * @param ctrl_sock  socket da ligação de controlo
 * @param ip_str     buffer para guardar IP em formato "a.b.c.d"
 * @param ip_max     tamanho máximo do buffer ip_str
 * @param port       ponteiro para guardar a porta calculada
 * @return 0 em sucesso, -1 em erro
 */
int ftp_enter_passive(int ctrl_sock, char *ip_str, size_t ip_max, int *port);

/**
 * Faz o download de um ficheiro usando uma ligação de dados já aberta.
 *
 * Fluxo típico:
 *  - enviar "RETR <remote_path>" na ligação de controlo
 *  - ler resposta 150 / 125 (transfer starting)
 *  - ler dados do socket de dados e gravar em local_filename
 *  - fechar socket de dados
 *  - ler resposta final 226 (transfer complete)
 *
 * @param ctrl_sock      socket de controlo (porta 21)
 * @param data_sock      socket de dados (ligado ao IP/porta do PASV)
 * @param remote_path    caminho remoto completo (ex: "pub/test/file.txt")
 * @param local_filename nome do ficheiro local (ex: "file.txt")
 * @return 0 em sucesso, -1 em erro
 */
int ftp_retrieve(int ctrl_sock, int data_sock,
                 const char *remote_path, const char *local_filename);

/**
 * Envia o comando QUIT e fecha a sessão FTP (nível de protocolo).
 * Não fecha o socket automaticamente aqui (podes decidir no main).
 *
 * @param ctrl_sock  socket da ligação de controlo
 * @return 0 em sucesso, -1 em erro
 */
int ftp_quit(int ctrl_sock);

#endif