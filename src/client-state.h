#include <netinet/in.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

#ifndef _CLIENT_STATE_H_
#define _CLIENT_STATE_H_

/**
 * @brief Container  of information relating to client <--> server operations.
 * Can be used to check where the connection goes to.
 *
 */
struct client_state {
    /**
     * @brief file descriptor connecting to the server
     *
     */
    int connection_fd;

    /**
     * @brief if the client is currently connected to the server. 0 = yes,
     * -1 = no
     *
     */
    int connected;

    /**
     * @brief hostname in ipv4 format, for display purposes
     *
     */
    char *ipv4_hostname;

    /**
     * @brief server connection information
     *
     */
    struct sockaddr_in serv;

    SSL     *ssl_fd;
    SSL_CTX *ssl_ctx;
} cs_state;

/**
 * @brief Reset the client state
 *
 */
void cs_reset();

void cs_free();

#endif