#ifndef URL_H
#define URL_H

#define DEFAULT_USER "rcom"
#define DEFAULT_PASS "rcom"

typedef struct
{
    char user[64];
    char password[64];
    char host[256];
    char path[512];
    char filename[256];
} ftp_url_t;

int parse_url(const char *url, ftp_url_t *out);

#endif