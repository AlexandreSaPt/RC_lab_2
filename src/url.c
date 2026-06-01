#include "url.h"

#include <string.h>
#include <stdio.h>

int parse_url(const char *url, ftp_url_t *out) {
    const char *p = url;
    const char prefix[] = "ftp://";

    if (strncmp(p, prefix, strlen(prefix)) != 0) {
        fprintf(stderr, "Invalid URL: must start with ftp://\n");
        return -1;
    }
    p += strlen(prefix);

    strcpy(out->user, DEFAULT_USER);
    strcpy(out->password, DEFAULT_PASS);

    const char *at = strchr(p, '@');
    const char *first_slash = strchr(p, '/');

    if (at != NULL && (first_slash == NULL || at < first_slash)) {

        const char *cred_sep = strchr(p, ':');

        if (cred_sep != NULL && cred_sep < at) {
            size_t user_len = cred_sep - p;
            size_t pass_len = at - cred_sep - 1;

            if (user_len >= sizeof(out->user) || pass_len >= sizeof(out->password)) {
                fprintf(stderr, "User or password too long\n");
                return -1;
            }

            memcpy(out->user, p, user_len);
            out->user[user_len] = '\0';

            memcpy(out->password, cred_sep + 1, pass_len);
            out->password[pass_len] = '\0';
        } else {
            size_t user_len = at - p;

            if (user_len >= sizeof(out->user)) {
                fprintf(stderr, "User too long\n");
                return -1;
            }

            memcpy(out->user, p, user_len);
            out->user[user_len] = '\0';
        }

        p = at + 1;
    }

    first_slash = strchr(p, '/');
    if (first_slash == NULL) {
        fprintf(stderr, "Invalid URL: missing path (ex: /file)\n");
        return -1;
    }

    size_t host_len = first_slash - p;
    if (host_len == 0 || host_len >= sizeof(out->host)) {
        fprintf(stderr, "Invalid or too long host\n");
        return -1;
    }

    memcpy(out->host, p, host_len);
    out->host[host_len] = '\0';

    const char *path_start = first_slash + 1;
    if (*path_start == '\0') {
        fprintf(stderr, "Empty path in URL\n");
        return -1;
    }

    if (strlen(path_start) >= sizeof(out->path)) {
        fprintf(stderr, "Path too long\n");
        return -1;
    }

    strcpy(out->path, path_start);

    const char *last_slash = strrchr(out->path, '/');
    const char *fname_start;

    if (last_slash != NULL) {
        fname_start = last_slash + 1;
    } else {
        fname_start = out->path;
    }

    if (*fname_start == '\0') {
        fprintf(stderr, "Empty filename (path ends with /)\n");
        return -1;
    }

    if (strlen(fname_start) >= sizeof(out->filename)) {
        fprintf(stderr, "Filename too long\n");
        return -1;
    }

    strcpy(out->filename, fname_start);

    return 0;
}