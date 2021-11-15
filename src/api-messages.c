#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "string.h"

static int argc_to_pos(char *s, int argc) {
    int pos = 0, args_seen = 0, pipes_seen_in_row = 0;

    int s_size = strlen(s);
    while (args_seen <= argc && pos < s_size) {
        char cur = s[pos];

        if (cur == '|') {
            pipes_seen_in_row++;
        } else {
            pipes_seen_in_row = 0;
        }

        if (pipes_seen_in_row == 2) {
            pipes_seen_in_row = 0;
            args_seen++;
        }

        pos++;
    }

    return pos;
}

void apim_add_param(char *s, char *param, int argc) {
    int start_pos = argc_to_pos(s, argc);

    s[start_pos] = '|';
    s[start_pos + 1] = '|';

    int param_length = strlen(param);
    for (int i = 0; i < param_length; i++) {
        s[start_pos + i + 2] = param[i];
    }
}

char *apim_create() {
    char *msg = malloc(sizeof(char) * 4096);
    return msg;
}

char *apim_read_param(char *s, int argc) {
    int arg_start_pos = argc_to_pos(s, argc);
    int s_size = strlen(s);

    int pipes_seen_in_row = 0;
    int end_pos = arg_start_pos;
    while (pipes_seen_in_row < 2 && end_pos < s_size) {
        char cur = s[end_pos];

        if (cur == '|') {
            pipes_seen_in_row++;
        } else {
            pipes_seen_in_row = 0;
        }

        end_pos++;
    }

    int pipe_account = 2;
    if (end_pos >= s_size) {
        pipe_account = 0;
    }

    char *param =
        malloc(sizeof(char) * (end_pos - arg_start_pos - pipe_account) + 1);
    memset(param, 0, (end_pos - arg_start_pos - pipe_account));

    for (int i = arg_start_pos; i < end_pos - pipe_account; i++) {
        param[i - arg_start_pos] = s[i];
    }

    return param;
}

int apim_count_args(char *s) {
    int size_s = strlen(s);
    int argc = 0;
    int pipes_seen_in_row = 0;
    for (int i = 0; i < size_s; i++) {
        char cur = s[i];

        if (cur == '|') {
            pipes_seen_in_row++;
        } else {
            pipes_seen_in_row = 0;
        }

        if (pipes_seen_in_row == 2) {
            argc++;
        }
    }
    return argc;
}

char **apim_parse_args(char *s) {
    int argc = apim_count_args(s);

    char **parsed_args = malloc(sizeof(char *) * argc);

    for (int i = 0; i < argc; i++) {
        char *param = apim_read_param(s, i);
        int   k = strlen(param);
        log_debug("apim_parse_args", "parsing arg %d as: %s", i, param);
        log_debug("apim_parse_args", "arg %d is strlen of: %d", i, k);
        parsed_args[i] = malloc(sizeof(char) * strlen(param));
        parsed_args[i] = param;
    }

    return parsed_args;
}