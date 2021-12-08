#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "api-messages.h"
#include "logger.h"
#include "server-commands.h"
#include "server-db.h"
#include "server-state.h"
#include "ssl-nonblock.h"
#include "utils.h"

//---- FUNCTION DECLARES -------------------------------------------------------
/**
 * @brief Print simple usage info about this app
 *
 */
static void usage();

/**
 * @brief Entry point for socket connection and processes
 *
 */
static void listen_for_connections();
static void create_server(uint16_t port);

int main(int argc, char **argv) {
    uint16_t port;

    // validate cmd arguments
    if (argc < 2 || (port = utils_parse_port(argv[1])) == -1) {
        usage();
        return 1;
    }

    // optionally enable debug mode if 2nd parameter was DEBUG
    log_set_debug_mode(1);
    if (argc == 3) {
        if (strcmp(argv[2], "DEBUG") == 0) {
            log_set_debug_mode(0);
            log_debug("main", "enabling debug mode");
        }
    }

    // initializations here
    ss_reset();
    cmdh_setup_server_commands();
    sdb_setup();

    // server spinup operations here
    create_server(port);
    bind(ss_state->server_fd, (struct sockaddr *)&ss_state->serv,
         sizeof(ss_state->serv));
    printf("Listening for connections on port %s\n", argv[2]);

    // setup listeners here
    listen_for_connections();

    // server cleanup operations here
    ss_free();
    cmdh_free_server_commands();
    sdb_free();

    return 0;
}

//---- FUNCTIONS ---------------------------------------------------------------
static void listen_for_connections() {
    listen(ss_state->server_fd, 5);

    // loop break conditions:
    //      - select FD fails
    //      - someone type's "q" into the server, graceful closure
    while (1) {
        fd_set readfds;

        // setting up select using STDIN, servers self fd, all connection fds
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(ss_state->server_fd, &readfds);
        int fdmax = ss_state->server_fd;

        for (int i = 0; i < SS_MAX_CHILDREN; i++) {
            int child_fd = ss_state->child_fd[i];
            if (child_fd != -1) {  // -1 = not connected
                FD_SET(child_fd, &readfds);
                if (child_fd > fdmax) {
                    fdmax = child_fd;
                }
            }
        }

        int s = select(fdmax + 1, &readfds, NULL, NULL, NULL);
        if (s < 0) {
            log_debug("listen_for_connections", "**select failed**");
            return;
        }

        // if servers self fd is set, that means we have an incomming
        // connection. save the FD as a child connection
        if (FD_ISSET(ss_state->server_fd, &readfds)) {
            // TODO: don't accept greater than SS_MAX_CHILDREN connections at a
            // time
            int cur_conn =
                accept(ss_state->server_fd, (struct sockaddr *)NULL, NULL);
            ss_add_child_connection(cur_conn);
        }

        for (int i = 0; i < SS_MAX_CHILDREN; i++) {
            int child_fd = ss_state->child_fd[i];
            int fd_loc = ss_get_fd_loc(child_fd);

            // if child connection exists and has incomming data, process
            if (child_fd != -1 && FD_ISSET(child_fd, &readfds) &&
                ssl_has_data(ss_state->ssl_fd[fd_loc])) {
                char *message;
                apim_capture_socket_msg(ss_state->ssl_fd[fd_loc], child_fd,
                                        &message);
                log_debug("listen_for_connections",
                          "recieved message \"%s\" from %d", message, child_fd);

                char *health_check = apim_create();
                apim_add_param(&health_check, "HEALTH", 0);
                apim_finish(&health_check);
                int n = ssl_block_write(ss_state->ssl_fd[fd_loc], child_fd,
                                        health_check, strlen(health_check));
                // int n = SSL_write(ss_state->ssl_fd[fd_loc], health_check,
                //   strlen(health_check));
                // int n = send(child_fd, health_check, strlen(health_check),
                // 0);
                log_debug("apim_capture_socket_msg", "n value on SEND: %d", n);
                if (n < 0) {
                    log_debug("apim_capture_socket_msg", "tunnel is dead");
                    cmdh_execute_command("||CLOSE", child_fd);
                } else if (message[0] == '\0') {  // default to close
                    cmdh_execute_command("||CLOSE", child_fd);
                } else {
                    cmdh_execute_command(message, child_fd);
                }
                log_debug("listen_for_connections", "end of message process\n");
            }
        }

        // if someone has typed something into the terminal, read that input and
        // process
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char *message = utils_capture_n_string(stdin, 4096);
            utils_clear_newlines(message);
            if (strcmp(message, "q") == 0) {
                log_debug("listen_for_connections", "closing server...");
                break;
            } else {
                printf("Type \"q\" to close server\n");
            }
        }
    }
}

static void create_server(uint16_t port) {
    ss_state->serv.sin_family = AF_INET;
    ss_state->serv.sin_port = htons(port);
    ss_state->serv.sin_addr.s_addr = INADDR_ANY;
    ss_state->server_fd = socket(AF_INET, SOCK_STREAM, 0);
}

static void usage() {
    printf("usage:\n");
    printf("\t [port] [DEBUG]\n");
}