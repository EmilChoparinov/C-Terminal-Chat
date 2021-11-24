#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api-messages.h"
#include "client-state.h"
#include "commands.h"
#include "logger.h"
#include "utils.h"

static struct cmd_command_list cmdc_commands;

int cmdc_exit(char **args, int argc) {
    log_debug("cmdc_exit", "logging out of %d", cs_state.connection_fd);

    char *msg = apim_create();
    apim_add_param(msg, "CLOSE", 0);
    send(cs_state.connection_fd, msg, strlen(msg), 0);

    close(cs_state.connection_fd);
    free(msg);
    return 1;
}

// TODO: login and register are virtually the same except on route calls
int cmdc_login(char **args, int argc) {
    log_debug("cmdc_exit", "do login");

    if (argc != 3) {
        printf("User login command invalid, do /help to see usage\n");
        return 0;
    }

    SHA256_CTX    ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char *)args[2], strlen(args[2]));
    SHA256_Final(md, &ctx);

    // TODO: make this a util function
    int  size = SHA256_DIGEST_LENGTH;
    char out[2 * size + 1];
    for (int i = 0; i < size; i++) {
        sprintf(out + (i * 2), "%02x", md[i]);
    }
    out[2 * size] = '\0';

    log_debug("cmdc_login", "output hash is: %s", out);

    // send register request
    char *msg = apim_create();
    apim_add_param(msg, "LOGIN", 0);
    apim_add_param(msg, args[1], 1);
    apim_add_param(msg, out, 2);
    send(cs_state.connection_fd, msg, strlen(msg), 0);
    free(msg);

    return 0;
}

int cmdc_logout(char **args, int argc) {
    log_debug("cmdc_logout", "entered logout routine");

    char *api_msg = apim_create();
    apim_add_param(api_msg, "LOGOUT", 0);
    send(cs_state.connection_fd, api_msg, strlen(api_msg), 0);
    free(api_msg);
    return 0;
}

int cmdc_register(char **args, int argc) {
    log_debug("cmdc_register", "do register");

    if (argc != 3) {
        printf("User register command invalid, do /help to see usage\n");
        return 0;
    }

    SHA256_CTX    ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char *)args[2], strlen(args[2]));
    SHA256_Final(md, &ctx);

    // TODO: make this a util function
    int  size = SHA256_DIGEST_LENGTH;
    char out[2 * size + 1];
    for (int i = 0; i < size; i++) {
        sprintf(out + (i * 2), "%02x", md[i]);
    }
    out[2 * size] = '\0';

    log_debug("cmdc_register", "output hash is: %s", out);

    // send register request
    char *msg = apim_create();
    apim_add_param(msg, "REGISTER", 0);
    apim_add_param(msg, args[1], 1);
    apim_add_param(msg, out, 2);
    send(cs_state.connection_fd, msg, strlen(msg), 0);
    free(msg);

    return 0;
}

int cmdc_users(char **args, int argc) {
    log_debug("cmdc_users", "listing users");
    return 0;
}

int cmdc_help(char **args, int argc) {
    log_debug("cmdc_help", "doing help");

    printf(
        "Below are a list of commands and a short description:\n\n \
/exit     - logout and leave the chat\n \
/login    - login into the chat client\n \
/register - create a new account\n \
/users    - display users\n");

    return 0;
}

int cmdc_global(char **args, int argc) {
    printf("%s\n", args[1]);
    return 0;
}

int cmdc_server_disconnected(char **args, int argc) {
    printf("Server has disconnected you.\n");
    return 1;
}

int cmdc_server_response(char **args, int argc) {
    log_debug("cmdc_server", "Receieved server response");
    if (argc > 0) {
        printf("%s\n", args[1]);
    }
    return 0;
}

void cmdc_setup_client_commands() {
    cmd_create_command_list(&cmdc_commands);

    /* CLIENT ONLY COMMANDS */
    cmd_register_command(&cmdc_commands, "/exit", &cmdc_exit);
    cmd_register_command(&cmdc_commands, "/login", &cmdc_login);
    cmd_register_command(&cmdc_commands, "/logout", &cmdc_logout);
    cmd_register_command(&cmdc_commands, "/register", &cmdc_register);
    cmd_register_command(&cmdc_commands, "/users", &cmdc_users);
    cmd_register_command(&cmdc_commands, "/help", &cmdc_help);

    /* SERVER RECEIVED COMMANDS */
    cmd_register_command(&cmdc_commands, "GLOBAL", &cmdc_global);
    cmd_register_command(&cmdc_commands, "SERVER_DISCONNECTED",
                         &cmdc_server_disconnected);
    cmd_register_command(&cmdc_commands, "SERV_RESPONSE",
                         &cmdc_server_response);
}

void cmdc_free_client_commands() { cmd_deregister(&cmdc_commands); }

int cmdc_execute_command(char *command) {
    log_debug("cmdc_execute_command", "executing command \"%s\"...", command);

    log_debug("cmdc_execute_command", "collections string arguments for \"%s\"",
              command);
    int    argc;
    char **args = utils_str_to_args(command, &argc);
    log_debug("cmdc_execute_command", "collected total %d arguments", argc);

    if (argc == 0) return 0;  // default empty to expected

    if (cmd_is_command(&cmdc_commands, args[0]) == 1) {
        printf("Invalid Command, do /help to view all commands\n");
        return 0;
    }

    int result = cmd_execute(&cmdc_commands, args[0], args, argc);

    // free arguments after use
    for (int i = 0; i < argc; i++) {
        free(args[i]);
    }
    free(args);

    return result;
}

int cmdc_execute_server_command(char *command) {
    utils_clear_newlines(command);
    log_debug("cmdc_execute_server_command", "executing command \"%s\"...",
              command);

    char **parsed_args = apim_parse_args(command);
    if (cmd_is_command(&cmdc_commands, parsed_args[0]) == 1) {
        log_debug("cmdc_execute_server_command",
                  "received invalid command from server %s", command);
        return 0;
    }

    int result = cmd_execute(&cmdc_commands, parsed_args[0], parsed_args,
                             apim_count_args(command));

    apim_free_args(parsed_args, apim_count_args(command));
    return result;
}