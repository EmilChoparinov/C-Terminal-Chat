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

int cmdc_exit(char **args) {
    log_debug("cmdc_exit", "logging out of %d", cs_state.connection_fd);

    char *msg = apim_create();
    apim_add_param(msg, "LOGOUT", 0);
    send(cs_state.connection_fd, msg, sizeof(msg), 0);

    close(cs_state.connection_fd);
    free(msg);
    return 1;
}

int cmdc_login(char **args) {
    log_debug("cmdc_exit", "do login");
    return 0;
}

int cmdc_register(char **args) {
    log_debug("cmdc_register", "do register");
    return 0;
}

int cmdc_users(char **args) {
    log_debug("cmdc_users", "listing users");
    return 0;
}

int cmdc_help(char **args) {
    log_debug("cmdc_help", "doing help");

    printf(
        "Below are a list of commands and a short description:\n\n \
/exit     - logout and leave the chat\n \
/login    - login into the chat client\n \
/register - create a new account\n \
/users    - display users\n");

    return 0;
}

int cmdc_global(char **args) {
    printf("%s\n", args[1]);
    return 0;
}

int cmdc_server_disconnected(char **args) {
    printf("Server has disconnected you.\n");
    return 1;
}

void cmdc_setup_client_commands() {
    cmd_create_command_list(&cmdc_commands);

    /* CLIENT ONLY COMMANDS */
    cmd_register_command(&cmdc_commands, "/exit", &cmdc_exit);
    cmd_register_command(&cmdc_commands, "/login", &cmdc_login);
    cmd_register_command(&cmdc_commands, "/register", &cmdc_register);
    cmd_register_command(&cmdc_commands, "/users", &cmdc_users);
    cmd_register_command(&cmdc_commands, "/help", &cmdc_help);

    /* SERVER RECEIVED COMMANDS */
    cmd_register_command(&cmdc_commands, "GLOBAL", &cmdc_global);
    cmd_register_command(&cmdc_commands, "SERVER_DISCONNECTED",
                         &cmdc_server_disconnected);
}

void cmdc_free_client_commands() { cmd_deregister(&cmdc_commands); }

int cmdc_execute_command(char *command) {
    log_debug("cmdc_execute_command", "executing command \"%s\"...", command);
    if (cmd_is_command(&cmdc_commands, command) == 1) {
        printf("Invalid Command, do /help to view all commands\n");
        return 0;
    }

    char *args_[1];
    return cmd_execute(&cmdc_commands, command, args_);
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

    int result = cmd_execute(&cmdc_commands, parsed_args[0], parsed_args);

    apim_free_args(parsed_args, apim_count_args(command));
    return result;
}