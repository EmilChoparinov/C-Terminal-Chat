#include "logger.h"

#include <stdarg.h>
#include <stdio.h>

void log_set_debug_mode(int mode) { log_state.debug_mode = mode; }

void log_debug(const char *fun_name, const char *format, ...) {
    if (log_state.debug_mode == 0) {
        va_list args;
        va_start(args, format);
        printf("[%s] ", fun_name);
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
}