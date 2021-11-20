#include "commands.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "string.h"

void cmd_create_command_list(struct cmd_command_list *command_list) {
    command_list->_i = 0;

    // initialize all names with an empty string
    for (int i = 0; i < CMD_COUNT; i++) {
        command_list->names[i] = "";
    }
}

int cmd_execute(struct cmd_command_list *commands, char *command, char **args,
                int argc) {
    log_debug("cmd_execute", "executing given command \"%s\"...", command);
    for (int i = 0; i < CMD_COUNT; i++) {
        log_debug("cmd_execute", "reading \"%s\" looking for \"%s\" (%d)",
                  commands->names[i], command,
                  strcmp(command, commands->names[i]));
        // if there is a match for the current iterated upon string, then that
        // is the proper command to execute
        if (strcmp(command, commands->names[i]) == 0) {
            log_debug("cmd_execute", "found command \"%s\"", command);
            return commands->executer[i](args, argc);
        }
    }
    log_debug("cmd_execute", "command not found, aborting");
    return 1;
}

int cmd_is_command(struct cmd_command_list *commands, char *command) {
    for (int i = 0; i < CMD_COUNT; i++) {
        log_debug("cmd_is_command", "checking command %s against %s",
                  commands->names[i], command);
        // if there is a match then return true as you found the command
        if (strcmp(command, commands->names[i]) == 0) {
            log_debug("cmd_is_command", "found command");
            return 0;
        }
    }
    return 1;
}

int cmd_has_command_prop(char *command) { return command[0] != '/'; }

void cmd_register_command(struct cmd_command_list *commands, char *command_name,
                          int (*func)(char **args, int argc)) {
    assert(commands->_i < CMD_COUNT);

    // save the string name and function pointer to the command list struct
    commands->names[commands->_i] = malloc(strlen(command_name) + 1);
    strcpy(commands->names[commands->_i], command_name);
    commands->executer[commands->_i] = func;
    commands->_i++;
}

void cmd_deregister(struct cmd_command_list *commands) {
    for (int i = 0; i < commands->_i; i++) {
        free(commands->names[i]);
    }
    commands->_i = 0;
}