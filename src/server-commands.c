#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "api-messages.h"
#include "commands.h"
#include "logger.h"
#include "server-messages.h"
#include "server-state.h"
#include "sql-inject-checker.h"
#include "time.h"

static struct cmd_command_list cmdh_commands;

// hacky but will let us access the client that sent us the current message. It
// should be a non-issue as our server is single-threaded anyway. This variable
// is set first thing when entering a command execution.
static int cur_fd;

int cmdh_logout(char **args, int argc) {
    log_debug("cmdh_logout", "closing connection to %d", cur_fd);
    ss_remove_child_connection(cur_fd);
    log_debug("cmdh_logout", "socket close success");
    return 0;
}

int cmdh_msg_global(char **args, int argc) {
    // create and format the current datetime to send
    time_t cur_date;
    time(&cur_date);
    char date[sizeof "2021-07-07T08:08:09Z"];
    strftime(date, sizeof(date), "%Y-%m-%d %I:%M:%S", localtime(&cur_date));
    char name[20] = "[NAME]";

    // print to server for server viewing reference
    printf("%s %s (%d): %s\n", date, name, cur_fd, args[1]);

    // format the message into `out`
    char out[4096];
    sprintf(out, "%s %s: %s\n", date, name, args[1]);

    // send the formatted message across all clients as this is the global chat
    char *api_msg = apim_create();
    apim_add_param(api_msg, "GLOBAL", 0);
    apim_add_param(api_msg, out, 1);
    sm_propogate_message(cur_fd, api_msg);

    // cleanups here
    free(api_msg);
    return 0;
}

int cmdh_register(char **args, int argc) {
    log_debug("cmdh_register", "size of password: %d", sizeof(args[2]));
    return 0;
}

void cmdh_setup_server_commands() {
    cmd_create_command_list(&cmdh_commands);

    // register server commands/requests here
    cmd_register_command(&cmdh_commands, "GLOBAL", &cmdh_msg_global);
    cmd_register_command(&cmdh_commands, "LOGOUT", &cmdh_logout);
    cmd_register_command(&cmdh_commands, "REGISTER", &cmdh_register);
}

int cmdh_execute_command(char *command, int from_fd) {
    cur_fd = from_fd;

    char **parsed_args = apim_parse_args(command);
    if (cmd_is_command(&cmdh_commands, parsed_args[0]) == 1) {
        printf("Invalid Server Command \"%s\"\n", command);
        return 0;
    }

    int result = cmd_execute(&cmdh_commands, parsed_args[0], parsed_args,
                             apim_count_args(command));

    apim_free_args(parsed_args, apim_count_args(command));

    return result;
}

void cmdh_free_server_commands() { cmd_deregister(&cmdh_commands); }