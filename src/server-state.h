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
struct server_state {
    int                server_fd;
    int                child_count;
    int                child_fd[SS_MAX_CHILDREN];
    int                child_pending[SS_MAX_CHILDREN];
    struct sockaddr_in serv;
} ss_state;

/**
 * @brief Resets the servers state to being empty, as if the program just
 * started execution.
 */
void ss_reset();

/**
 * @brief Get if the child is child_pending. A valid file description within
 * range is required
 *
 * @param fd child file descriptor from [0,15]
 * @return int child child_pending state
 */
int ss_get_child_pending(int fd);

/**
 * @brief Get if the current child is instanced under a specific file descriptor
 *
 * @param fd child file descriptor from [0,15]
 * @return int 0 if child DNE, 1 if child exists
 */
int ss_child_instance_exists(int fd);

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

#endif