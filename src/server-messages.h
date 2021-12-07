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

int sm_global_save_message(char *message, int uid, char *created_at);
#endif