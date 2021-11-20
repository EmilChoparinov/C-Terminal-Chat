#include <stdint.h>

/**
 * @brief Given a string of characters representing a port, converts into
 * uint16_t
 */
uint16_t utils_parse_port(const char *port);

/**
 * @brief Converts given hostname to ipv4 address.
 *
 * @param hostname hostname to convert
 * @return int ipv4 address if possible
 */
char *utils_hostname_to_ipv4(const char *hostname);

/**
 * @brief Clean all newlines out of a string
 *
 * @param str_to_clean the string to remove newlines from
 */
void utils_clear_newlines(char *str_to_clean);

/**
 * @brief Converts a string to an argument array
 *
 * @param str_of_args string to convert
 * @param out_count the amount of arguments
 * @return char** an argument array
 */
char **utils_str_to_args(char *str_of_args, int *out_count);