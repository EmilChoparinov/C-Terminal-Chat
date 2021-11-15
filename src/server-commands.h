#ifndef _SERVER_COMMANDS_H_
#define _SERVER_COMMANDS_H_
#include "commands.h"

/**
 * @brief Run initializer for the client commands
 *
 */
void cmdh_setup_client_commands();

/**
 * @brief Executes a given command
 *
 * @param command command name
 * @return int return 0 for expected output, return 1 for failed output
 */
int cmdh_execute_command(char *command, int from_fd);

#endif
