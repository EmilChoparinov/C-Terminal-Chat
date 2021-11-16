/**
 * @brief send a server message to all open connections and exclude a single
 * connection
 *
 * @param from_fd the fd that the message came from
 * @param message the message to relay
 */
void sm_propogate_message(int from_fd, char *message);