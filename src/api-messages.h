
#include <openssl/ssl.h>

/**
 * @brief add a parameter to a string message
 *
 * @param s the string message
 * @param param the param to add
 */
void apim_add_param(char **s, char *param, int argc);

/**
 * @brief get a parameter found within a string message
 *
 * @param s the string message
 * @param pos the argument position
 * @return char* a new string of that param
 */
char *apim_read_param(char *s, int argc);

/**
 * @brief Create an api message
 *
 * @return char*
 */
char *apim_create();

/**
 * @brief Closes an api message and gets ready to send over
 *
 */
void apim_finish(char **s);

/**
 * @brief Capture a message within a socket
 *
 * @param fd the socket to capture
 * @return char* message returned
 */
void apim_capture_socket_msg(SSL *ssl, int fd, char **m_out);

/**
 * @brief Get the count of arguments within a message
 *
 * @param s message to count args of
 * @return int the arg count
 */
int apim_count_args(char *s);

/**
 * @brief Parse a message into a 2d array
 *
 * @param s message to parse
 * @return char** 2d array of arguments
 */
char **apim_parse_args(char *s);

/**
 * @brief free memory associated with an argumented api message
 *
 * @param args arguments created by `apim_parse_args` to free
 * @param argc count of arguments
 */
void apim_free_args(char **args, int argc);

/**
 * @brief Checks the validity of a string
 * 
 * @param s the api string
 * @return int 0 = good, 1 = bad
 */
int apim_is_valid(char *s);