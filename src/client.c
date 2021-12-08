#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <signal.h>
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
#include "ssl-nonblock.h"
#include "utils.h"

static void usage();
static void ctrl_c_handler(int sig);
static void setup_server_connection(uint16_t port, const char *host);
static int  do_server_connection();
static void establish_server_listener();

int main(int argc, char **argv) {
    /* signals to catch */
    signal(SIGINT, ctrl_c_handler);

    uint16_t port;

    // validate cmd arguments
    if (argc < 3 || (port = utils_parse_port(argv[2])) == -1) {
        usage();
        return 1;
    }

    // optionally enable debug mode if 3rd paramter was DEBUG
    log_set_debug_mode(1);
    if (argc == 4) {
        if (strcmp(argv[3], "DEBUG") == 0) {
            log_set_debug_mode(0);
            log_debug("main", "enabling debug mode");
        }
    }

    // initializations here
    cs_reset();
    cmdc_setup_client_commands();

    // client spinup operations here
    setup_server_connection(port, argv[1]);
    if (do_server_connection() != 0) {
        return 1;
    }

    // setup listeners here
    establish_server_listener();

    // client cleanup here
    cs_free();
    close(cs_state.connection_fd);
    cmdc_free_client_commands();

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
    printf("Connected to %s:%d\n", cs_state.ipv4_hostname,
           cs_state.serv.sin_port);

    set_nonblock(cs_state.connection_fd);

    SSL_set_fd(cs_state.ssl_fd, cs_state.connection_fd);
    ssl_block_connect(cs_state.ssl_fd, cs_state.connection_fd);

    log_debug("establish_server_listener", "SSL context established");

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

        if (FD_ISSET(cs_state.connection_fd, &readfds) &&
            ssl_has_data(cs_state.ssl_fd)) {
            log_debug("establish_server_listener", "received incoming message");
            char *message;
            apim_capture_socket_msg(cs_state.ssl_fd, cs_state.connection_fd,
                                    &message);
            log_debug("establish_server_listener", "parsed message as '%s'",
                      message);
            int should_exit = cmdc_execute_server_command(message);
            if (should_exit != 0) {
                return;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            log_debug("establish_server_listener", "recieved user input");
            char *message;
            message = utils_capture_n_string(stdin, 4096);
            utils_clear_newlines(message);
            if (strlen(message) > 0 && message[0] == '@') {
                cmdc_private_dm(message);
            } else {
                if (cmd_has_command_prop(message) == 0) {
                    int should_exit = cmdc_execute_command(message);
                    if (should_exit != 0) {
                        return;
                    }
                } else {
                    char *api_msg = apim_create();
                    apim_add_param(&api_msg, "GLOBAL", 0);
                    apim_add_param(&api_msg, message, 1);
                    apim_finish(&api_msg);
                    log_debug("establish_server_connection",
                              "sending api msg:\n%s", api_msg);
                    // int n = send(cs_state.connection_fd, api_msg,
                    //              strlen(api_msg), 0);
                    // int n = SSL_write(ssl, api_msg, strlen(api_msg));
                    int n =
                        ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd,
                                        api_msg, strlen(api_msg));
                    if (n < 0) {
                        log_debug("establish_server_connection",
                                  "error on writing");
                        printf("Server is unavailable, closing client");
                        exit(0);
                    }
                    free(api_msg);
                }
            }
            free(message);
        }
    }
}

static void ctrl_c_handler(int sig) {
    log_debug("ctrl_c_handler", "caught a ctrl operation, cleaning up...");
    char *msg = apim_create();
    apim_add_param(&msg, "CLOSE", 0);
    send(cs_state.connection_fd, msg, strlen(msg), 0);
    apim_finish(&msg);
    free(msg);
    cs_free();
    close(cs_state.connection_fd);
    cmdc_free_client_commands();
    exit(0);
}

static void usage() {
    printf("usage:\n");
    printf("\t [host] [port]\n");
}