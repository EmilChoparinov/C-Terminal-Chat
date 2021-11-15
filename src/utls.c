#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void utils_clear_newlines(char *str_to_clean) {
    str_to_clean[strcspn(str_to_clean, "\n")] = 0;
}