#include <stdint.h>
#include <stdio.h>

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

/**
 * @brief Duplicate a given string
 *
 * @param src_str source string
 * @return char* duplicated string
 */
char *utils_dup_str(char *src_str);

/**
 * @brief Given an unsigned char array of bytes, convert it to a readable string
 *
 * @param md the hash
 * @param size the size of the hash
 */
char *utils_md_to_string(unsigned char *md, int size);

/**
 * @brief capture an undefined length of a string from the user
 *
 * @param fp the file to capture from
 * @param size the intial size
 * @return char* the captured string
 */
char *utils_capture_n_string(FILE *fp, size_t size);

/**
 * @brief prepend a string to a string
 *
 * @param prep the prepend value
 * @param dest the destination string
 */
void utils_prepend(char *prep, char *dest);

/**
 * @brief append N sized string to a given string destination
 *
 * @param dest string to get str appended to
 * @param str str to append
 */
void utils_append(char **dest, const char *str);