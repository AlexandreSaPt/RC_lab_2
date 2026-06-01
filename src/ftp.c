#include "ftp.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int ftp_read_line(int sockfd, char *buf, size_t max)
{
    size_t i = 0;
    char c;
    ssize_t n;

    while (i < max - 1)
    {
        n = read(sockfd, &c, 1);
        if (n < 0)
        {
            perror("read()");
            return -1;
        }
        if (n == 0)
        {
            if (i == 0)
                return -1;
            break;
        }

        buf[i++] = c;

        if (c == '\n')
            break;
    }

    buf[i] = '\0';
    return (int)i;
}

int ftp_read_reply(int sockfd, char *response, size_t max, int *out_code)
{
    while (1)
    {
        if (ftp_read_line(sockfd, response, max) < 0)
        {
            fprintf(stderr, "Error reading FTP response\n");
            return -1;
        }

        if (strlen(response) < 4)
            continue;

        int code = ftp_get_reply_code(response);
        if (code < 0)
            continue;

        if (response[3] == '-')
            continue;

        if (out_code)
            *out_code = code;
        return 0;
    }
}

int ftp_send_cmd(int sockfd, const char *cmd, char *response, size_t max)
{
    char buffer[1024];

    int len = snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
    if (len < 0 || (size_t)len >= sizeof(buffer))
    {
        fprintf(stderr, "FTP command too large\n");
        return -1;
    }

    printf(">> %s\n", cmd);

    if (write(sockfd, buffer, len) < 0)
    {
        perror("write()");
        return -1;
    }

    int code;
    if (ftp_read_reply(sockfd, response, max, &code) < 0)
        return -1;

    printf("<< %s", response);
    return 0;
}

int ftp_get_reply_code(const char *response)
{
    int code = -1;

    if (sscanf(response, "%3d", &code) != 1)
        return -1;

    return code;
}

int ftp_login(int ctrl_sock, const char *user, const char *password)
{
    char response[1024];
    char cmd[256];
    int code;
    if (ftp_read_reply(ctrl_sock, response, sizeof(response), &code) < 0)
    {
        fprintf(stderr, "Error reading initial banner\n");
        return -1;
    }
    printf("<< %s", response);

    if (code != FTP_CODE_SERVICE_READY)
    {
        fprintf(stderr, "Server is not ready (expected 220, received %d)\n", code);
        return -1;
    }
    snprintf(cmd, sizeof(cmd), "USER %s", user);
    if (ftp_send_cmd(ctrl_sock, cmd, response, sizeof(response)) < 0)
    {
        fprintf(stderr, "Error sending USER\n");
        return -1;
    }
    code = ftp_get_reply_code(response);
    if (code == FTP_CODE_LOGIN_SUCCESS)
    {
        return 0;
    }
    if (code != FTP_CODE_USERNAME_OK)
    {
        fprintf(stderr, "USER rejected (code %d)\n", code);
        return -1;
    }
    snprintf(cmd, sizeof(cmd), "PASS %s", password);
    if (ftp_send_cmd(ctrl_sock, cmd, response, sizeof(response)) < 0)
    {
        fprintf(stderr, "Error sending PASS\n");
        return -1;
    }
    code = ftp_get_reply_code(response);
    if (code == FTP_CODE_LOGIN_SUCCESS)
    {
        return 0;
    }
    if (code == FTP_CODE_LOGIN_FAIL)
    {
        fprintf(stderr, "Login failed (530)\n");
    }
    else
    {
        fprintf(stderr, "Unexpected code after PASS: %d\n", code);
    }
    return -1;
}

int ftp_enter_passive(int ctrl_sock, char *ip_str, size_t ip_max, int *port)
{
    char response[1024];

    if (ftp_send_cmd(ctrl_sock, "PASV", response, sizeof(response)) < 0)
    {
        fprintf(stderr, "Error sending PASV\n");
        return -1;
    }
    int code = ftp_get_reply_code(response);
    if (code != FTP_CODE_ENTERING_PASSIVE)
    {
        fprintf(stderr, "Unexpected response to PASV: %d\n", code);
        return -1;
    }
    const char *p = strchr(response, '(');
    const char *q = strchr(response, ')');
    if (!p || !q || q <= p + 1)
    {
        fprintf(stderr, "Invalid format in PASV response\n");
        return -1;
    }
    char inside[128];
    size_t len = q - p - 1;
    if (len >= sizeof(inside))
    {
        fprintf(stderr, "PASV content too long\n");
        return -1;
    }

    memcpy(inside, p + 1, len);
    inside[len] = '\0';

    int h1, h2, h3, h4, p1, p2;
    if (sscanf(inside, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6)
    {
        fprintf(stderr, "Failed to parse PASV numbers\n");
        return -1;
    }

    int n = snprintf(ip_str, ip_max, "%d.%d.%d.%d", h1, h2, h3, h4);
    if (n < 0 || (size_t)n >= ip_max)
    {
        fprintf(stderr, "Buffer ip_str too small\n");
        return -1;
    }

    *port = p1 * 256 + p2;

    return 0;
}

int ftp_retrieve(int ctrl_sock, int data_sock,
                 const char *remote_path, const char *local_filename)
{
    char response[1024];
    char cmd[512];

    snprintf(cmd, sizeof(cmd), "RETR %s", remote_path);
    if (ftp_send_cmd(ctrl_sock, cmd, response, sizeof(response)) < 0)
    {
        fprintf(stderr, "Error sending RETR\n");
        return -1;
    }

    int code = ftp_get_reply_code(response);
    if (code != FTP_CODE_OPENING_DATA &&
        code != FTP_CODE_DATA_ALREADY_OPEN)
    {
        if (code == FTP_CODE_FILE_UNAVAILABLE)
        {
            fprintf(stderr, "Remote file unavailable (550)\n");
        }
        else
        {
            fprintf(stderr, "Unexpected code after RETR: %d\n", code);
        }
        return -1;
    }

    FILE *f = fopen(local_filename, "wb");
    if (!f)
    {
        perror("fopen()");
        return -1;
    }

    char buffer[4096];
    ssize_t n;

    while ((n = read(data_sock, buffer, sizeof(buffer))) > 0)
    {
        if (fwrite(buffer, 1, n, f) != (size_t)n)
        {
            perror("fwrite()");
            fclose(f);
            close(data_sock);
            return -1;
        }
    }

    if (n < 0)
    {
        perror("read() from data connection");
        fclose(f);
        close(data_sock);
        return -1;
    }

    fclose(f);
    close(data_sock);

    if (ftp_read_reply(ctrl_sock, response, sizeof(response), &code) < 0)
    {
        fprintf(stderr, "Error reading final response after RETR\n");
        return -1;
    }
    if (code != FTP_CODE_TRANSFER_COMPLETE)
    {
        fprintf(stderr, "Transfer did not complete successfully (code %d)\n", code);
        return -1;
    }

    return 0;
}

int ftp_quit(int ctrl_sock)
{
    char response[1024];

    if (ftp_send_cmd(ctrl_sock, "QUIT", response, sizeof(response)) < 0)
    {
        fprintf(stderr, "Error sending QUIT\n");
        return -1;
    }

    int code = ftp_get_reply_code(response);
    if (code != FTP_CODE_CLOSING_CTRL)
    {
        fprintf(stderr, "Unexpected code after QUIT: %d\n", code);
        return -1;
    }

    return 0;
}
