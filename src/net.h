#ifndef NET_H
#define NET_H

#include <netinet/in.h>

/**
 * Resolve um hostname para um endereço IPv4 (sockaddr_in).
 * 
 * @param hostname  ex: "ftp.netlab.fe.up.pt"
 * @param addr      struct sockaddr_in de saída
 * @param port      porta TCP em host order (ex: 21)
 * @return 0 em sucesso, -1 em erro
 */
int resolve_host(const char *hostname, struct sockaddr_in *addr, int port);

/**
 * Abre uma ligação TCP a partir de uma struct sockaddr_in já preenchida.
 *
 * @param addr  endereço e porta do servidor
 * @return descritor de socket (>0) em sucesso, -1 em erro
 */
int tcp_connect(struct sockaddr_in *addr);

#endif // NET_H