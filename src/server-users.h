#ifndef _SERVER_USERS_H_
#define _SERVER_USERS_H_

#define SU_INVALID_LOGIN 1 /* User login is invalid */
#define SU_EXISTS 2        /* User already exists */
#define SU_INVALID 3       /* Data is invalid in some manner */
#define SU_LOGIN_EXISTS 4  /* User is already logged in */
#define SU_DNE 5           /* User does not exist in DB */

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
int su_validate_login(char *username, char *password);

/**
 * @brief Check if username already exists
 *
 * @param username username to check
 * @return int 1 if does exist, 0 if does not exist
 */
int su_has_user(char *username);

int su_get_uid(char *username);

#endif