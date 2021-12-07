#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

uint16_t utils_parse_port(const char *port) {
    if (port == NULL) return -1;

    errno = 0;
    char *end_port;
    long  p_port = strtol(port, &end_port, 0);

    if (!p_port && errno) return -1;
    if (*end_port) return -1;

    if (p_port < 0 || p_port > 65535) return -1;

    return p_port;
}

char *utils_hostname_to_ipv4(const char *hostname) {
    struct hostent  *host;
    struct in_addr **addresses;

    if ((host = gethostbyname(hostname)) == NULL) {
        return "";
    }

    while (host) {
        if (host->h_addrtype == AF_INET && host->h_addr_list) {
            addresses = (struct in_addr **)host->h_addr_list;
            int i = 0;
            while (addresses[i] == NULL) {
                i++;
            }
            return inet_ntoa(*addresses[i]);
        }
        host = gethostent();
    }
    return "";
}

char **utils_str_to_args(char *arg_str, int *argc) {
    log_debug("utils_str_to_args", "recieved string \"%s\"", arg_str);
    int arg_count = 0;
    int i = 0;
    while (arg_str[i] != '\0') {
        if (arg_str[i] == ' ') arg_count++;
        i++;
    }
    arg_count++;
    *argc = arg_count;

    log_debug("utils_str_to_args", "found total args: %d", arg_count);

    char **args = malloc(sizeof(char *) * (arg_count + 1));
    int    arg_pos = 0;
    i = 0;
    while (arg_str[i] != '\0' && arg_pos < arg_count) {
        int end_arg_pos = 0;
        while (arg_str[i + end_arg_pos] != ' ' &&
               arg_str[i + end_arg_pos] != '\n' &&
               arg_str[i + end_arg_pos] != '\0') {
            end_arg_pos++;
        }

        log_debug("utils_str_to_args", "allocating %d for arg %d", end_arg_pos,
                  arg_pos);
        log_debug("utils_str_to_args", "str is size %d", strlen(arg_str));
        args[arg_pos] = malloc(sizeof(char) * (end_arg_pos + 2));
        if (args[arg_pos] == NULL) {
            log_debug("utils_str_to-args", "malloc failed?");
        }

        int copy_start = i;
        while (i < copy_start + end_arg_pos) {
            args[arg_pos][i - copy_start] = arg_str[i];
            i++;
        }

        args[arg_pos][end_arg_pos] = '\0';

        log_debug("utils_str_to-args", "made is here");
        arg_pos++;
        i++;
    }
    log_debug("utils_str_to-args", "finished str to args");
    return args;
}

void utils_clear_newlines(char *str_to_clean) {
    str_to_clean[strcspn(str_to_clean, "\n")] = 0;
}

char *utils_dup_str(char *src_str) {
    char *str;
    char *p;
    int   len = 0;

    while (src_str[len]) len++;
    str = malloc(len + 1);
    p = str;
    while (*src_str) *p++ = *src_str++;
    *p = '\0';
    return str;
}

char *utils_capture_n_string(FILE *fp, size_t size) {
    char  *str;
    int    k;
    size_t fp_len = 0;
    str = realloc(NULL, sizeof(*str) * size);
    if (!str) {
        return str;
    }
    while (EOF != (k = fgetc(fp)) && k != '\n') {
        str[fp_len++] = k;
        if (fp_len == size) {
            str = realloc(str, sizeof(*str) * (size += 256));
            if (!str) {
                return str;
            }
        }
    }
    str[fp_len++] = '\0';
    return realloc(str, sizeof(*str) * fp_len);
}

void utils_prepend(char *prep, const char *dest) {
    size_t len = strlen(dest);
    memmove(prep + len, prep, strlen(prep) + 1);
    memcpy(prep, dest, len);
    log_debug("utils_prepend", "prepended '%s' to '%s' with output '%s'", prep,
              dest, dest);
}