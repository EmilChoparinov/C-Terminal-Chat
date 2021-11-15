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