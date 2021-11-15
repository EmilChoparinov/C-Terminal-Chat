#include "commands.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "string.h"

void cmd_create_command_list(struct cmd_command_list *command_list) {
    command_list->_i = 0;
    for (int i = 0; i < CMD_COUNT; i++) {
        command_list->names[i] = "";
    }
}

int cmd_execute(struct cmd_command_list *commands, char *command, char **args) {
    for (int i = 0; i < CMD_COUNT; i++) {
        printf("[cmd_execute] reading \"%s\" looking for \"%s\" (%d)\n",
               commands->names[i], command,
               strcmp(command, commands->names[i]));
        if (strcmp(command, commands->names[i]) == 0) {
            printf("[cmd_execute] found command \"%s\"\n", command);
            return commands->executer[i](args);
        }
    }
    printf("[cmd_execute] command not found, aborting\n");
    return 1;
}

int cmd_is_command(struct cmd_command_list *commands, char *command) {
    for (int i = 0; i < CMD_COUNT; i++) {
        printf("[cmd_is_command] checking command %s against %s\n",
               commands->names[i], command);
        if (strcmp(command, commands->names[i]) == 0) {
            printf("[cmd_is_command] found command\n");
            return 0;
        }
    }
    return 1;
}

int cmd_has_command_prop(char *command) { return command[0] != '/'; }

void cmd_register_command(struct cmd_command_list *commands, char *command_name,
                          int (*func)(char **args)) {
    assert(commands->_i < CMD_COUNT);

    commands->names[commands->_i] = malloc(strlen(command_name) + 1);
    strcpy(commands->names[commands->_i], command_name);
    commands->executer[commands->_i] = func;
    commands->_i++;
}