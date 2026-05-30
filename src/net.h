#ifndef NET_H
#define NET_H

#include <netinet/in.h>

/**
 * Resolves a hostname to an IPv4 address (sockaddr_in).
 * 
 * @param hostname  ex: "ftp.netlab.fe.up.pt"
 * @param addr      output struct sockaddr_in
 * @param port      TCP port in host order (ex: 21)
 * @return 0 on success, -1 on error
 */
int resolve_host(const char *hostname, struct sockaddr_in *addr, int port);

/**
 * Opens a TCP connection from an already filled struct sockaddr_in.
 *
 * @param addr  server address and port
 * @return socket descriptor (>0) on success, -1 on error
 */
int tcp_connect(struct sockaddr_in *addr);

#endif // NET_H