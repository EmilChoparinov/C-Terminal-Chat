#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "logger.h"
#include "server-state.h"

void sm_propogate_message(int from_fd, char *message) {
    int *connections = ss_get_active_connections();
    int  connection_size = ss_get_active_size();

    log_debug("sm_propogate_message", "size of current connections: %d",
              connection_size);

    // this loop will:
    //      - go through each active connection
    //      - send the message to each connection that is **not**
    //          * from the connection itself
    //          * the server
    for (int i = 0; i < connection_size; i++) {
        log_debug("sm_propogate_message",
                  "reading for connection %d as pos %d...", connections[i], i);
        int cur_fd = connections[i];
        if (cur_fd != from_fd && cur_fd != ss_state->server_fd) {
            log_debug("sm_propogate_message", "sending message to %d", cur_fd);
            send(cur_fd, message, strlen(message), 0);
        }
    }

    free(connections);
}