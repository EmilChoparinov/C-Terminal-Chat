#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/**
 * @brief The total amount of allowed connections to this server.
 */
#define CMD_COUNT 16

struct cmd_command_list {
    int _i;
    int (*executer[CMD_COUNT])(char **args, int argc);
    char *names[CMD_COUNT];
};

/**
 * @brief Create a command list object
 *
 * @return cmd_command_list& command list object
 */
void cmd_create_command_list(struct cmd_command_list *command_list);

/**
 * @brief Main entry point for executing commands
 *
 * @param commands structure of commands to call
 * @param command command string
 * @return int 1 if unexpected occurance or program halt
 */
int cmd_execute(struct cmd_command_list *commands, char *command, char **args,
                int argc);

/**
 * @brief Check if a command string is part of the command list
 *
 * @param commands structure of commands to check with
 * @param command command string
 * @return int 0 if is command, 1 if not command
 */
int cmd_is_command(struct cmd_command_list *commands, char *command);

/**
 * @brief Register a command into the `cmd_command_list` struct.
 *
 * @param commands command struct
 * @param command_name name of the command
 * @param func pointer to the command function
 */
void cmd_register_command(struct cmd_command_list *commands, char *command_name,
                          int (*func)(char **args, int argc));

/**
 * @brief Free commands and clean struct
 *
 * @param commands the struct of commands to clean
 */
void cmd_deregister(struct cmd_command_list *commands);

/**
 * @brief Check if a command follows the correct command format
 *
 * @param command the command string
 * @return int 0 is true, 1 if false
 */
int cmd_has_command_prop(char *command);

#endif