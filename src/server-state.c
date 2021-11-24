
#include "server-state.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api-messages.h"
#include "logger.h"
#include "server-messages.h"

void ss_reset() {
    ss_state = malloc(sizeof(*ss_state));
    ss_state->server_fd = -1;
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        ss_state->child_fd[i] = -1;
        ss_state->active_users[i] = calloc(1, strlen(""));
        ss_state->active_users[i] = "\0";
    }
}

void ss_remove_child_connection(int fd) {
    close(fd);

    // find where the FD is stored within our connection collection, and reset
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        if (ss_state->child_fd[i] == fd) {
            ss_state->child_fd[i] = -1;
            ss_state->child_pending[i] = 0;
            return;
        }
    }
}

int ss_add_child_connection(int fd) {
    // find next available FD location, starting from 0
    log_debug("ss_add_child_connection", "adding fd: %d", fd);
    int fd_loc = 0;
    while (ss_state->child_fd[fd_loc] != -1) {
        fd_loc++;
    }
    assert(fd_loc < SS_MAX_CHILDREN);

    // add child connection to next open location
    log_debug("ss_add_child_connection", "adding at loc: %d fd: %d", fd_loc,
              fd);
    ss_state->child_fd[fd_loc] = fd;
    return fd_loc;
}

unsigned int ss_get_active_size() {
    unsigned int count = 0;
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        if (ss_state->child_fd[i] != -1) {
            count++;
        }
    }
    log_debug("ss_get_active_size", "size is: %d", count);
    return count;
}

int *ss_get_active_connections() {
    // array of active file descriptors ready to be populated
    int *fds = malloc(ss_get_active_size() * sizeof(unsigned int));
    // fds_i is the localized index for fds
    int fds_i = 0;
    // fd_loc is the index of all children
    int fd_loc = 0;
    while (fd_loc < SS_MAX_CHILDREN) {
        log_debug("ss_get_active_connections", "at %d value is: %d", fd_loc,
                  ss_state->child_fd[fd_loc]);
        if (ss_state->child_fd[fd_loc] != -1) {
            fds[fds_i] = ss_state->child_fd[fd_loc];
            fds_i++;
        }
        fd_loc++;
    }

    return fds;
}

void ss_free() {
    // since this will delete the current servers instance, we would like to
    // send a notice to each client that we are going offline and they should
    // attempt a reconnect.
    char *api_msg = apim_create();
    apim_add_param(api_msg, "SERVER_DISCONNECTED\n", 0);
    sm_propogate_message(ss_state->server_fd, api_msg);
    free(api_msg);

    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        int cur_fd = ss_state->child_fd[i];
        if (cur_fd != -1) {
            ss_remove_child_connection(cur_fd);
        }
    }
    close(ss_state->server_fd);
}

int ss_is_fd_logged_in(int fd) {
    log_debug("ss_is_fd_logged_in", "in fd logged checking with fd: %d", fd);
    int fd_loc = 0;
    while (ss_state->child_fd[fd_loc] != fd && fd_loc < SS_MAX_CHILDREN) {
        fd_loc++;
    }

    log_debug("ss_is_fd_logged_in", "located fd at %d", fd_loc);
    log_debug("ss_is_fd_logged_in", "username at loc is %s",
              ss_state->active_users[fd_loc]);
    if (strcmp(ss_state->active_users[fd_loc], "") == 0) {
        log_debug("ss_is_fd_logged_in", "determined user is not logged in");
        return 1;
    }
    log_debug("ss_is_fd_logged_in", "determined user is logged in");
    return 0;
}

int ss_is_user_logged_in(char *username) {
    log_debug("ss_is_user_logged_in", "checking if user \"%s\" is logged in",
              username);
    int fd_loc = 0;
    while (fd_loc != SS_MAX_CHILDREN &&
           strcmp(ss_state->active_users[fd_loc], username) != 0) {
        log_debug("ss_is_user_logged_in",
                  "starting string comparer on \"%s\" with \"%s\"",
                  ss_state->active_users[fd_loc], username);
        fd_loc++;
    }

    if (fd_loc == SS_MAX_CHILDREN) {
        return 1;
    }
    return 0;
}

void ss_login_fd(int fd, char *username) {
    log_debug("ss_login_fd", "applying user \"%s\" to FD %d", username, fd);
    int fd_loc = 0;
    while (ss_state->child_fd[fd_loc] != fd && fd_loc < SS_MAX_CHILDREN) {
        fd_loc++;
    }

    log_debug("ss_login_fd", "found location at %d", fd_loc);
    if (fd_loc == SS_MAX_CHILDREN) return;

    log_debug("ss_login_fd", "applying username to %d", fd_loc);
    ss_state->active_users[fd_loc] = calloc(1, strlen(username));
    strcpy(ss_state->active_users[fd_loc], username);
    log_debug("ss_login_fd", "username applied as %s",
              ss_state->active_users[fd_loc]);
}

void ss_logout_fd(int fd) {
    log_debug("ss_logout_fd", "entered logout");

    int fd_loc = 0;
    while (ss_state->child_fd[fd_loc] != fd && fd_loc < SS_MAX_CHILDREN) {
        fd_loc++;
    }

    if (fd_loc == SS_MAX_CHILDREN) return;

    ss_state->active_users[fd_loc] = calloc(1, strlen(""));
    ss_state->active_users[fd_loc] = "\0";
}