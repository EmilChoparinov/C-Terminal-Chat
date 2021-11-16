#ifndef _LOGGER_H_
#define _LOGGER_H_

/**
 * @brief log state struct for tweaking logging on server or client
 * applications, both share this library.
 *
 */
struct logger_state {
    /* flag with 0 enable debugging, 1 to disable debugging logs */
    int debug_mode;
} log_state;

/**
 * @brief set the debug mode
 *
 * @param mode 0 to enable debugging, 1 to disable debugging logs
 */
void log_set_debug_mode(int mode);

/**
 * @brief print a debug log to the console
 *
 * @param fun_name the name of the function calling this log
 * @param format format string
 * @param ... format string arguments
 */
void log_debug(const char *fun_name, const char *format, ...);

#endif