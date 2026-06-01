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

// ---- General errors ----
#define FTP_CODE_FILE_UNAVAILABLE    550  

/**
 * Reads a line from the FTP connection (control connection),
 * terminated in '\n' (server sends "\\r\\n").
 *
 * @param sockfd  TCP socket descriptor (control)
 * @param buf     output buffer
 * @param max     maximum buffer size
 * @return number of bytes read (>0) or -1 on error
 */
int ftp_read_line(int sockfd, char *buf, size_t max);

/**
 * Sends an FTP command (without CRLF) and reads a line of response.
 * Example: cmd = "USER anonymous"
 *
 * @param sockfd    control socket
 * @param cmd       command string (without "\\r\\n")
 * @param response  buffer for server response
 * @param max       maximum buffer response size
 * @return 0 on success, -1 on error
 */
int ftp_send_cmd(int sockfd, const char *cmd, char *response, size_t max);

/**
 * Extracts the numeric code (3 digits) from the beginning of an FTP response.
 * Example: "220 Service ready" -> 220
 *
 * @param response  string with complete response
 * @return code (>=100) on success, -1 on error (no 3 digits at start)
 */
int ftp_get_reply_code(const char *response);

/**
 * Performs LOGIN to FTP server using USER / PASS.
 *
 * Typical flow:
 *  - read first line from server (220 ...)
 *  - send USER <user>
 *  - if code 331, send PASS <pass>
 *  - verify code 230 (login OK)
 *
 * @param ctrl_sock  control connection socket (port 21)
 * @param user       username (ex: "anonymous")
 * @param password   password (ex: "anonymous@")
 * @return 0 on success, -1 on error (login failed)
 */
int ftp_login(int ctrl_sock, const char *user, const char *password);

/**
 * Enters PASSIVE mode (PASV) and gets IP and port
 * for the data connection.
 *
 * Flow:
 *  - send "PASV"
 *  - read response "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
 *  - build IP string "h1.h2.h3.h4"
 *  - calculate port p1*256 + p2
 *
 * @param ctrl_sock  control connection socket
 * @param ip_str     buffer to store IP in format "a.b.c.d"
 * @param ip_max     maximum buffer size ip_str
 * @param port       pointer to store calculated port
 * @return 0 on success, -1 on error
 */
int ftp_enter_passive(int ctrl_sock, char *ip_str, size_t ip_max, int *port);

/**
 * Downloads a file using an already open data connection.
 *
 * Typical flow:
 *  - send "RETR <remote_path>" on control connection
 *  - read response 150 / 125 (transfer starting)
 *  - read data from data socket and save in local_filename
 *  - close data socket
 *  - read final response 226 (transfer complete)
 *
 * @param ctrl_sock      control socket (port 21)
 * @param data_sock      data socket (connected to PASV IP/port)
 * @param remote_path    complete remote path (ex: "pub/test/file.txt")
 * @param local_filename local file name (ex: "file.txt")
 * @return 0 on success, -1 on error
 */
int ftp_retrieve(int ctrl_sock, int data_sock,
                 const char *remote_path, const char *local_filename);

/**
 * Sends the QUIT command and closes FTP session (protocol level).
 * Does not close socket automatically here (you can decide in main).
 *
 * @param ctrl_sock  control connection socket
 * @return 0 on success, -1 on error
 */
int ftp_quit(int ctrl_sock);

#endif