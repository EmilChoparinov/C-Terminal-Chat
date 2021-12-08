#ifndef _SERVER_DB_H_
#define _SERVER_DB_H_

#include <sqlite3.h>

sqlite3 *db_conn;

/**
 * @brief Setup routine for the SQLite3 database to be used by the server
 *
 * @return int 0 for success, 1 for no success
 */
int sdb_setup();

/**
 * @brief Free all data related to the current SQLite3 instance
 *
 */
void sdb_free();

/**
 * @brief Load some sql from the file system
 *
 * @param query the outputed query
 * @param path the local path location
 *
 * @return int 0 if success, 1 if not
 */
int sdb_load_static_sql(char **query, char *path);

/**
 * @brief Execute an sql query and get the results of said query in a callback
 *
 * @param mute mute error messages
 * @param query SQL query string
 * @param callback SQLite3 callback function
 *
 * @return int 0 if success, 1 if not
 */
int sdb_execute(int mute, char *query,
                int (*callback)(void *, int, char **, char **));

#endif