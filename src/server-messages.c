#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "server-state.h"

void sm_propogate_message(int from_fd, char *message, int size) {
    int *connections = ss_get_active_connections();
    int  connection_size = ss_get_active_size();
    printf("[sm_propogate_message] size of current connections: %d\n",
           connection_size);

    for (int i = 0; i < connection_size; i++) {
        printf(
            "[sm_propogate_message] reading for connection %d as pos %d...\n",
            connections[i], i);
        int cur_fd = connections[i];
        if (cur_fd != from_fd && cur_fd != ss_state.server_fd) {
            printf("[sm_propogate_message] sending message to %d\n", cur_fd);
            send(cur_fd, message, strlen(message), 0);
        }
    }

    free(connections);
}