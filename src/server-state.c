
#include "server-state.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"

void ss_reset() {
    memset(&ss_state, 0, sizeof(ss_state));
    ss_state.server_fd = -1;
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        ss_state.child_fd[i] = -1;
    }
}

int ss_get_child_pending(int fd) {
    assert(fd >= 0 && fd < SS_MAX_CHILDREN);

    int fd_exists = ss_child_instance_exists(fd);
    assert(!fd_exists);

    return ss_state.child_pending[fd];
}

int ss_child_instance_exists(int fd) {
    if (fd < 0 && fd >= SS_MAX_CHILDREN) return 0;

    return (*ss_state.child_fd) + 1;
}

void ss_remove_child_connection(int fd) {
    close(fd);
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        if (ss_state.child_fd[i] == fd) {
            ss_state.child_fd[i] = -1;
            ss_state.child_pending[i] = 0;
            return;
        }
    }
}

int ss_add_child_connection(int fd) {
    /* find next available FD location */
    log_debug("ss_add_child_connection", "adding fd: %d", fd);
    int fd_loc = 0;
    while (ss_state.child_fd[fd_loc] != -1) {
        fd_loc++;
    }
    assert(fd_loc < SS_MAX_CHILDREN);

    /* set child connection */
    log_debug("ss_add_child_connection", "adding at loc: %d fd: %d", fd_loc, fd);
    ss_state.child_fd[fd_loc] = fd;
    return fd_loc;
}

int *ss_get_active_connections() {
    int *fds = malloc(ss_get_active_size());

    int fds_i = 0;
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

void ss_free() {
    for (int i = 0; i < SS_MAX_CHILDREN; i++) {
        int cur_fd = ss_state.child_fd[i];
        if (cur_fd != -1) {
            ss_remove_child_connection(cur_fd);
        }
    }
    close(ss_state.server_fd);
}