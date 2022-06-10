#include "server-db.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger.h"

// poop
int sdb_setup() {
    int is_db_new = access("secure-chat.db", F_OK);

    int resp = sqlite3_open("secure-chat.db", &db_conn);
    if (resp) {
        log_debug("sdb_setup", "unable to open db");
        sqlite3_close(db_conn);
        return 1;
    }
    if (is_db_new != 0) {
        char *create_tables;

        int r = sdb_load_static_sql(&create_tables, "db-scripts/create.sql");
        if (r != 0) return r;
        r = sdb_execute(1, create_tables, NULL);
        if (r != 0) return r;
    }
    return 0;
}

void sdb_free() { sqlite3_close(db_conn); }

int sdb_load_static_sql(char **query, char *path) {
    log_debug("sdb_load_static_sql", "reading \"%s\"...", path);
    FILE *fp;
    long  size;

    fp = fopen(path, "r");
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    *query = calloc(1, size + 1);
    if (!*query) {
        fclose(fp);
        log_debug("sdb_load_static_sql", "memory alloc failed");
        return 1;
    }

    if (fread(*query, size, 1, fp) != 1) {
        fclose(fp);
        free(*query);
        log_debug("sdb_load_static_sql", "read failed");
        return 1;
    }
    return 0;
}

int sdb_execute(int mute, char *query,
                int (*callback)(void *, int, char **, char **)) {
    log_debug("sdb_execute", "executing \"%s\"\n...", query);

    char *err;
    int   resp = sqlite3_exec(db_conn, query, callback, NULL, &err);
    if (mute != 0 && resp != SQLITE_OK) {
        log_debug("sdb_execute", "query execution failed.\nSQL ERROR: %s", err);
        return 1;
    }
    log_debug("sdb_execute", "query executed sucessfully");
    return 0;
}