#include "server-messages.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "logger.h"
#include "server-db.h"
#include "server-state.h"
#include "sql-inject-checker.h"

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

int sm_global_save_message(char *message, int uid, char *created_at) {
    log_debug("sm_global_save_message", "entered global message save");
    int is_msg_ok = sic_is_sql_ok(message);
    if (is_msg_ok != 0) {
        return MSG_SAVE_FAILED;
    }

    sqlite3_stmt *query;

    int sql_err = sqlite3_prepare_v2(
        db_conn,
        "INSERT INTO message(from_user,message,is_dm,created_at) "
        "VALUES(?,?,?,?)",
        -1, &query, NULL);
    if (sql_err != SQLITE_OK) {
        log_debug("sm_global_save_message", "SQL Error Occured: Code %s",
                  sql_err);
        return MSG_SAVE_FAILED;
    }

    sqlite3_bind_int(query, 1, uid);
    sqlite3_bind_text(query, 2, message, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(query, 3, 1);
    sqlite3_bind_text(query, 4, created_at, -1, SQLITE_TRANSIENT);

    sqlite3_step(query);
    sqlite3_finalize(query);
    return 0;
}