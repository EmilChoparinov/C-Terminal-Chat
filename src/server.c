#include <netinet/in.h>  // get AF_INET address family
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "logger.h"
#include "server-commands.h"
#include "server-state.h"
#include "utils.h"

static void usage();
static void listen_for_connections();
static void create_server(uint16_t port);

int main(int argc, char **argv) {
    uint16_t port;

    /* validate arguments */
    if (argc != 2 || (port = utils_parse_port(argv[1])) == -1) {
        usage();
        return 1;
    }

    /* server state setup */
    ss_reset();
    log_set_debug_mode(0);
    cmdh_setup_client_commands();

    /* server spinup operations */
    create_server(port);
    bind(ss_state.server_fd, (struct sockaddr *)&ss_state.serv,
         sizeof(ss_state.serv));
    printf("Listening for connections on port %d\n", port);

    /* setup listeners */
    listen_for_connections();

    /* server cleanup operations */
    ss_free();
    return 0;
}

static void listen_for_connections() {
    listen(ss_state.server_fd, 5);

    while (1) {
        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(ss_state.server_fd, &readfds);
        int fdmax = ss_state.server_fd;

        for (int i = 0; i < SS_MAX_CHILDREN; i++) {
            int child_fd = ss_state.child_fd[i];
            if (child_fd != -1) {
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

        if (FD_ISSET(ss_state.server_fd, &readfds)) {
            int cur_conn =
                accept(ss_state.server_fd, (struct sockaddr *)NULL, NULL);
            ss_add_child_connection(cur_conn);
        }

        for (int i = 0; i < SS_MAX_CHILDREN; i++) {
            int child_fd = ss_state.child_fd[i];

            if (child_fd != -1 && FD_ISSET(child_fd, &readfds)) {
                char message[4096] = "";
                recv(child_fd, message, sizeof(message), 0);
                log_debug("listen_for_connections", "recieved message \"%s\"",
                          message);
                cmdh_execute_command(message, child_fd);
                log_debug("listen_for_connections", "end of message process\n");
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char message[4096] = "";
            fgets(message, sizeof(message), stdin);
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
    ss_state.serv.sin_family = AF_INET;
    ss_state.serv.sin_port = htons(port);
    ss_state.serv.sin_addr.s_addr = INADDR_ANY;
    ss_state.server_fd = socket(AF_INET, SOCK_STREAM, 0);
}

static void usage() {
    printf("usage:\n");
    printf("\t [port]\n");
}