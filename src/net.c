#include "net.h"

#include <stdio.h>          
#include <string.h>        
#include <unistd.h>        
#include <sys/socket.h>    
#include <netdb.h>        
#include <arpa/inet.h>      

int resolve_host(const char *hostname, struct sockaddr_in *addr, int port) {
    struct hostent *h;
    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        return -1;
    }
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    memcpy(&(addr->sin_addr.s_addr), h->h_addr, h->h_length);

    return 0;
}

int tcp_connect(struct sockaddr_in *addr) {
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("connect()");
        close(sockfd);
        return -1;
    }

    return sockfd;
}