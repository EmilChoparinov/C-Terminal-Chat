#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "api-messages.h"
#include "client-commands.h"
#include "client-state.h"
#include "logger.h"
#include "utils.h"

static void usage();
static void setup_server_connection(uint16_t port, const char *host);
static int  do_server_connection();
static void establish_server_listener();

int main(int argc, char **argv) {
    uint16_t port;

    /* validate arguments */
    if (argc != 3 || (port = utils_parse_port(argv[2])) == -1) {
        usage();
        return 1;
    }

    log_set_debug_mode(0);
    cs_reset();

    setup_server_connection(port, argv[1]);
    if (do_server_connection() != 0) {
        return 1;
    }

    printf("Connected to %s:%d\n", cs_state.ipv4_hostname, port);
    cmdc_setup_client_commands();
    establish_server_listener();

    close(cs_state.connection_fd);

    return 0;
}

static void setup_server_connection(uint16_t port, const char *host) {
    assert(port);
    assert(host);

    cs_state.serv.sin_addr.s_addr = INADDR_ANY;
    cs_state.connection_fd = socket(AF_INET, SOCK_STREAM, 0);
    cs_state.serv.sin_family = AF_INET;
    cs_state.serv.sin_port = htons(port);

    char *ipv4 = utils_hostname_to_ipv4(host);
    cs_state.ipv4_hostname = ipv4;
    inet_pton(AF_INET, ipv4, &cs_state.serv.sin_addr);
}

static int do_server_connection() {
    cs_state.connection_fd = socket(AF_INET, SOCK_STREAM, 0);

    int v = connect(cs_state.connection_fd, (struct sockaddr *)&cs_state.serv,
                    sizeof(cs_state.serv));
    if (v != 0) {
        perror("[do_server_connection]: cannot connect to server:");
        return 1;
    }
    return 0;
}

static void establish_server_listener() {
    while (1) {
        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(cs_state.connection_fd, &readfds);
        int fdmax = cs_state.connection_fd;

        int s = select(fdmax + 1, &readfds, NULL, NULL, NULL);
        if (s < 0) {
            log_debug("establish_server_listener", "**error: select failed**");
            return;
        }

        char message[4096] = "";
        if (FD_ISSET(cs_state.connection_fd, &readfds)) {
            int n = recv(cs_state.connection_fd, message, sizeof(message), 0);
            if (n < 0) {
                printf("error on reading");
            } else {
                int should_exit = cmdc_execute_server_command(message);
                if (should_exit != 0) {
                    return;
                }
                // printf("%s", message);
            }
        }

        char *api_msg;
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(message, sizeof(message), stdin);
            utils_clear_newlines(message);
            if (cmd_has_command_prop(message) == 0) {
                int should_exit = cmdc_execute_command(message);
                if (should_exit != 0) {
                    return;
                }
            } else {
                api_msg = apim_create();
                apim_add_param(api_msg, "GLOBAL", 0);
                apim_add_param(api_msg, message, 1);
                int n =
                    send(cs_state.connection_fd, api_msg, strlen(api_msg), 0);
                if (n < 0) {
                    log_debug("establish_server_connection",
                              "error on writing");
                }
                memset(api_msg, '\0', strlen(api_msg));
            }
        }
    }
}

static void usage() {
    printf("usage:\n");
    printf("\t [host] [port]\n");
}