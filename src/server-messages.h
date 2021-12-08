#ifndef _SERVER_MESSAGES_H_
#define _SERVER_MESSAGES_H_

/**
 * @brief send a server message to all open connections and exclude a single
 * connection
 *
 * @param from_fd the fd that the message came from
 * @param message the message to relay
 */

#define MSG_SAVE_FAILED 3 /* message did not get sent to the db */

#include <sqlite3.h>

void sm_propogate_message(int from_fd, char *message);

/**
 * @brief save a message into the sqlite db that is meant to be public to
 * everyone
 *
 * @param message message to save
 * @param uid who the message came from
 * @param created_at a creation date
 * @return int
 */
int sm_global_save_message(char *message, int uid, char *created_at);

/**
 * @brief save a message into the sqlite db that is meant to be private between
 * two users
 *
 * @param message message to save
 * @param from_uid who the message came from
 * @param to_uid who the message is for
 * @param created_at a creation date
 * @return int
 */
int sm_pm_save_message(char *message, int from_uid, int to_uid,
                       char *created_at);
#endif