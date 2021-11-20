#ifndef _SERVER_USERS_H_
#define _SERVER_USERS_H_

/**
 * @brief Registers a user with this server
 *
 * @param username
 * @param password
 * @param response callback with the db response
 */
int su_register_user(char *username, char *password);

/**
 * @brief Validate a users login attempt
 *
 * @param username
 * @param password
 * @param response callback with the db response
 */
void su_validate_login(char *username, char *password,
                       void (*response)(int res));

/**
 * @brief Check if username already exists
 *
 * @param username username to check
 * @return int 0 if does exist, 1 if does not exist
 */
int su_has_user(char *username);

#endif