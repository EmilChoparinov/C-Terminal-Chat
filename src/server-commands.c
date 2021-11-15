#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "api-messages.h"
#include "commands.h"
#include "logger.h"
#include "server-messages.h"
#include "server-state.h"
#include "time.h"

struct cmd_command_list cmdh_commands;

static int cur_fd;

int cmdh_logout(char **args) {
    log_debug("cmdh_logout", "closing connection to %d", cur_fd);
    close(cur_fd);
    log_debug("cmdh_logout", "socket close success");
    ss_remove_child_connection(cur_fd);
    return 0;
}

int cmdh_msg_global(char **args) {
    time_t cur_date;
    time(&cur_date);
    char date[sizeof "2021-07-07T08:08:09Z"];

    strftime(date, sizeof(date), "%Y-%m-%d %I:%M:%S", localtime(&cur_date));
    char name[20] = "[NAME]";
    printf("%s %s (%d): %s\n", date, name, cur_fd, args[1]);
    char out[4096];
    sprintf(out, "%s %s: %s\n", date, name, args[1]);

    char *api_msg = apim_create();

    apim_add_param(api_msg, "GLOBAL", 0);
    apim_add_param(api_msg, out, 1);

    sm_propogate_message(cur_fd, api_msg);

    free(api_msg);
    return 0;
}

void cmdh_setup_client_commands() {
    cmd_create_command_list(&cmdh_commands);

    cmd_register_command(&cmdh_commands, "GLOBAL", &cmdh_msg_global);
    cmd_register_command(&cmdh_commands, "LOGOUT", &cmdh_logout);
}

int cmdh_execute_command(char *command, int from_fd) {
    cur_fd = from_fd;

    char **parsed_args = apim_parse_args(command);
    if (cmd_is_command(&cmdh_commands, parsed_args[0]) == 1) {
        printf("Invalid Command, do /help to view all commands\n");
        return 0;
    }

    int result = cmd_execute(&cmdh_commands, parsed_args[0], parsed_args);

    apim_free_args(parsed_args);

    return result;
}