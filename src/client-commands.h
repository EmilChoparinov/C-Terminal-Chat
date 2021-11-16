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

/**
 * @brief Executes responses from server
 * 
 * @param command command name
 * @return int return 0 for expected output, return 1 for failed output
 */
int cmdc_execute_server_command(char *command);

/**
 * @brief Free all data relating to server commands
 * 
 */
void cmdc_free_client_commands();

#endif
