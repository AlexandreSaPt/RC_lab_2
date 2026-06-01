#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>       

#include "url.h"
#include "net.h"
#include "ftp.h"

int main(int argc, char *argv[]) {
    ftp_url_t u;
    struct sockaddr_in addr_ctrl;
    struct sockaddr_in addr_data;
    int ctrl_sock, data_sock;
    char pasv_ip[64];
    int pasv_port;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[user:pass@]host/path/file\n", argv[0]);
        return 1;
    }

    // Parse URL
    if (parse_url(argv[1], &u) != 0) {
        fprintf(stderr, "URL parsing error\n");
        return 1;
    }

    // Resolve hostname
    if (resolve_host(u.host, &addr_ctrl, 21) < 0) {
        fprintf(stderr, "Error in resolve_host\n");
        return 1;
    }

    // Open TCP control connection
    ctrl_sock = tcp_connect(&addr_ctrl);
    if (ctrl_sock < 0) {
        fprintf(stderr, "Error connecting to FTP server (control)\n");
        return 1;
    }

    // 4. FTP Login
    if (ftp_login(ctrl_sock, u.user, u.password) < 0) {
        fprintf(stderr, "Error in FTP login\n");
        close(ctrl_sock);
        return 1;
    }

    // 5. Enter PASSIVE mode
    if (ftp_enter_passive(ctrl_sock, pasv_ip, sizeof(pasv_ip), &pasv_port) < 0) {
        fprintf(stderr, "Error entering PASSIVE mode\n");
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    // Prepare sockaddr_in for data connection
    memset(&addr_data, 0, sizeof(addr_data));
    addr_data.sin_family = AF_INET;
    addr_data.sin_port = htons(pasv_port);

    if (inet_aton(pasv_ip, &addr_data.sin_addr) == 0) {
        fprintf(stderr, "Invalid IP returned by PASV: %s\n", pasv_ip);
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    // 6. Open TCP data connection
    data_sock = tcp_connect(&addr_data);
    if (data_sock < 0) {
        fprintf(stderr, "Error connecting to FTP data connection\n");
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    // 7. Download file
    if (ftp_retrieve(ctrl_sock, data_sock, u.path, u.filename) < 0) {
        fprintf(stderr, "Error downloading file\n");
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    printf("Download completed successfully: %s\n", u.filename);

    // 8. Close FTP session
    if (ftp_quit(ctrl_sock) < 0) {
        fprintf(stderr, "Warning: error sending QUIT\n");
    }

    close(ctrl_sock);
    return 0;
}

/*
make

./download ftp://ftp.netlab.fe.up.pt/ftp/teste.txt
./download ftp://ftp.netlab.fe.up.pt/ftp/ubuntu-24.04.2-desktop-amd64.iso
```

+-------------------+                 +---------------------+
|      CLIENT       |                 |       SERVER        |
|                   |                 |     (FTP Server)    |
| parse_url()       |                 |                     |
| resolve_host()    | -- TCP:21 -->   | 220 Service ready   |
| tcp_connect()     | <------------   |                     |
| ftp_read_line()   |                 |                     |
| ftp_send_cmd()    | USER ---------->| 331 Password req.   |
| ftp_send_cmd()    | PASS ---------->| 230 Login OK        |
| ftp_send_cmd()    | PASV ---------->| 227 (h1,h2...)      |
| parse PASV        |                 |                     |
| tcp_connect(data) | -- TCP:N -----> | 150 Opening...      |
| read(data)        | <--- FILE ----- | 226 Transfer OK     |
| QUIT              |---------------> | 221 Goodbye         |
| close sockets     |                 |                     |
+-------------------+                 +---------------------+

```
*/