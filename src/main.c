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

    // Parse do URL
    if (parse_url(argv[1], &u) != 0) {
        fprintf(stderr, "Erro de parsing da URL\n");
        return 1;
    }

    // Resolver hostname
    if (resolve_host(u.host, &addr_ctrl, 21) < 0) {
        fprintf(stderr, "Erro em resolve_host\n");
        return 1;
    }

    // Abrir ligação TCP de controle
    ctrl_sock = tcp_connect(&addr_ctrl);
    if (ctrl_sock < 0) {
        fprintf(stderr, "Erro a conectar ao servidor FTP (controle)\n");
        return 1;
    }

    // 4. Login FTP
    if (ftp_login(ctrl_sock, u.user, u.password) < 0) {
        fprintf(stderr, "Erro no login FTP\n");
        close(ctrl_sock);
        return 1;
    }

    // 5. Entrar em modo PASSIVE
    if (ftp_enter_passive(ctrl_sock, pasv_ip, sizeof(pasv_ip), &pasv_port) < 0) {
        fprintf(stderr, "Erro ao entrar em modo PASSIVE\n");
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    // Preparar sockaddr_in para a ligação de dados
    memset(&addr_data, 0, sizeof(addr_data));
    addr_data.sin_family = AF_INET;
    addr_data.sin_port = htons(pasv_port);

    if (inet_aton(pasv_ip, &addr_data.sin_addr) == 0) {
        fprintf(stderr, "IP inválido devolvido pelo PASV: %s\n", pasv_ip);
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    // 6. Abrir ligação TCP de dados
    data_sock = tcp_connect(&addr_data);
    if (data_sock < 0) {
        fprintf(stderr, "Erro a conectar à ligação de dados FTP\n");
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    // 7. Fazer o download do ficheiro
    if (ftp_retrieve(ctrl_sock, data_sock, u.path, u.filename) < 0) {
        fprintf(stderr, "Erro ao fazer download do ficheiro\n");
        ftp_quit(ctrl_sock);
        close(ctrl_sock);
        return 1;
    }

    printf("Download concluído com sucesso: %s\n", u.filename);

    // 8. Terminar sessão FTP
    if (ftp_quit(ctrl_sock) < 0) {
        fprintf(stderr, "Aviso: erro ao enviar QUIT\n");
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