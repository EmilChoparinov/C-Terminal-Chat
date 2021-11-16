
/**
 * @brief add a parameter to a string message
 *
 * @param s the string message
 * @param param the param to add
 */
void apim_add_param(char *s, char *param, int argc);

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