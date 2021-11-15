#include <netinet/in.h>
#ifndef _CLIENT_STATE_H_
#define _CLIENT_STATE_H_

/**
 * @brief Container  of information relating to client <--> server operations.
 * Can be used to check where the connection goes to.
 *
 */
struct client_state {
    int                connection_fd;
    char*              ipv4_hostname;
    struct sockaddr_in serv;
} cs_state;

#endif