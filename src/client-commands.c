#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "api-messages.h"
#include "client-state.h"
#include "commands.h"
#include "utils.h"

struct cmd_command_list cmdc_commands;

int cmdc_exit(char **args) {
    printf("[cmdc_exit] logging out of %d\n", cs_state.connection_fd);

    char *msg = apim_create();
    apim_add_param(msg, "LOGOUT", 0);
    send(cs_state.connection_fd, msg, sizeof(msg), 0);

    close(cs_state.connection_fd);
    return 1;
}

int cmdc_login(char **args) {
    printf("[cmdc_exit] do login\n");
    return 0;
}

int cmdc_register(char **args) {
    printf("[cmdc_register] do register\n");
    return 0;
}

int cmdc_users(char **args) {
    printf("[cmdc_users] listing users\n");
    return 0;
}

int cmdc_help(char **args) {
    printf("[cmdc_help] doing help\n");

    printf(
        "Below are a list of commands and a short description:\n\n \
/exit     - logout and leave the chat\n \
/login    - login into the chat client\n \
/register - create a new account\n \
/users    - display users\n");

    return 0;
}

void cmdc_setup_client_commands() {
    cmd_create_command_list(&cmdc_commands);

    cmd_register_command(&cmdc_commands, "/exit", &cmdc_exit);
    cmd_register_command(&cmdc_commands, "/login", &cmdc_login);
    cmd_register_command(&cmdc_commands, "/register", &cmdc_register);
    cmd_register_command(&cmdc_commands, "/users", &cmdc_users);
    cmd_register_command(&cmdc_commands, "/help", &cmdc_help);
}

int cmdc_execute_command(char *command) {
    printf("[cmdc_execute_command] executing command \"%s\"...\n", command);
    if (cmd_is_command(&cmdc_commands, command) == 1) {
        printf("Invalid Command, do /help to view all commands\n");
        return 0;
    }

    char *args_[1];
    return cmd_execute(&cmdc_commands, command, args_);
}