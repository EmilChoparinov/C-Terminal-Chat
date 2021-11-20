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
               arg_str[i + end_arg_pos] != '\n') {
            end_arg_pos++;
        }

        log_debug("utils_str_to_args", "allocating %d for arg %d", end_arg_pos,
                  arg_pos);
        args[arg_pos] = malloc(sizeof(char) * (end_arg_pos + 1));

        int copy_start = i;
        while (i < copy_start + end_arg_pos) {
            args[arg_pos][i - copy_start] = arg_str[i];
            i++;
        }

        args[arg_pos][end_arg_pos] = '\0';

        arg_pos++;
        i++;
    }
    return args;
}

void utils_clear_newlines(char *str_to_clean) {
    str_to_clean[strcspn(str_to_clean, "\n")] = 0;
}