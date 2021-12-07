#ifndef _SERVER_STATE_H_
#define _SERVER_STATE_H_

#include <netinet/in.h>

/**
 * @brief The total amount of allowed connections to this server.
 */
#define SS_MAX_CHILDREN 16

/**
 * @brief Container of information relating to server operations. Can be used to
 * check connections, remove connections, etc.
 */
typedef struct server_state {
    int                server_fd;
    int                child_count;
    int                child_fd[SS_MAX_CHILDREN];
    int                child_pending[SS_MAX_CHILDREN];
    char              *active_users[SS_MAX_CHILDREN];
    struct sockaddr_in serv;
} server_state;

server_state *ss_state;

/**
 * @brief Resets the servers state to being empty, as if the program just
 * started execution.
 */
void ss_reset();

/**
 * @brief Remove a child connection from the server state using its file
 * descriptor
 *
 * @param fd child file descriptor from [0,15]
 */
void ss_remove_child_connection(int fd);

/**
 * @brief Add a child connection from the server state and apply it via a file
 * descriptor.
 *
 * CAREFUL: this will overwrite any old state without cleanup.
 *
 * @param fd child file descriptor to add
 */
int ss_add_child_connection(int fd);

/**
 * @brief Get all active child connection file descriptors
 *
 * @return int* array of file descriptors
 */
int *ss_get_active_connections();

/**
 * @brief Get the amount of currently active connections, useful for
 * displaying how full the server is at a current moment
 *
 * @return uint the amount of connections
 */
unsigned int ss_get_active_size();

/**
 * @brief Frees any memory related to the server state and closes all currently
 * active socket connections
 *
 */
void ss_free();

/**
 * @brief Check if a connection is logged in and is allowed to send messages etc
 *
 * @param fd The file descriptor to check
 * @return int 0 if true, 1 if false
 */
int ss_is_fd_logged_in(int fd);

/**
 * @brief Check if a user is logged in already
 *
 * @param username The username to check if its logged in
 * @return int 0 if true, 1 if false
 */
int ss_is_user_logged_in(char *username);

/**
 * @brief Set a connection to logged in
 *
 * @param fd the file descriptor to login with
 * @param username the user that will be logged in here
 */
void ss_login_fd(int fd, char *username);

/**
 * @brief Logout a user
 *
 * @param fd the file descriptor to logout of
 */
void ss_logout_fd(int fd);

/**
 * @brief Get a string list of all active users
 *
 */
char *ss_get_active_user_list();

/**
 * @brief Get the active amount of users logged in and connected
 *
 * @return int the count of users
 */
int ss_get_active_user_count();

/**
 * @brief Get the username by the fd
 *
 * @param fd fd to lookup
 * @return char* the username
 */
char *ss_get_username(int fd);

#endif