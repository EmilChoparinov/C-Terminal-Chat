#ifndef _CLIENT_COMMANDS_H_
#define _CLIENT_COMMANDS_H_
#include "commands.h"

/**
 * @brief Run initializer for the client commands
 *
 */
void cmdc_setup_client_commands();

/**
 * @brief Executes a given command
 *
 * @param command command name
 * @return int return 0 for expected output, return 1 for failed output
 */
int cmdc_execute_command(char *command);

#endif
