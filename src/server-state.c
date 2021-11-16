
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
    memset(&ss_state, 0, sizeof(ss_state));
    ss_state.server_fd = -1;
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        ss_state.child_fd[i] = -1;
    }
}

void ss_remove_child_connection(int fd) {
    close(fd);

    // find where the FD is stored within our connection collection, and reset
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        if (ss_state.child_fd[i] == fd) {
            ss_state.child_fd[i] = -1;
            ss_state.child_pending[i] = 0;
            return;
        }
    }
}

int ss_add_child_connection(int fd) {
    // find next available FD location, starting from 0
    log_debug("ss_add_child_connection", "adding fd: %d", fd);
    int fd_loc = 0;
    while (ss_state.child_fd[fd_loc] != -1) {
        fd_loc++;
    }
    assert(fd_loc < SS_MAX_CHILDREN);

    // add child connection to next open location
    log_debug("ss_add_child_connection", "adding at loc: %d fd: %d", fd_loc,
              fd);
    ss_state.child_fd[fd_loc] = fd;
    return fd_loc;
}

unsigned int ss_get_active_size() {
    unsigned int count = 0;
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        if (ss_state.child_fd[i] != -1) {
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
                  ss_state.child_fd[fd_loc]);
        if (ss_state.child_fd[fd_loc] != -1) {
            fds[fds_i] = ss_state.child_fd[fd_loc];
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
    sm_propogate_message(ss_state.server_fd, api_msg);
    free(api_msg);

    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        int cur_fd = ss_state.child_fd[i];
        if (cur_fd != -1) {
            ss_remove_child_connection(cur_fd);
        }
    }
    close(ss_state.server_fd);
}