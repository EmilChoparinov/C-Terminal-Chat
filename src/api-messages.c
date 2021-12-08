#include <ctype.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "logger.h"
#include "ssl-nonblock.h"
#include "string.h"
#include "utils.h"

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

void apim_add_param(char **s, char *param, int argc) {
    char *arg = malloc(sizeof(char) * 1);
    arg[0] = '\0';
    utils_append(&arg, "||");
    utils_append(&arg, param);
    utils_append(s, arg);
    log_debug("apim_add_param", "added arg '%s'", arg);
    log_debug("apim_add_param", "new message is: %s", *s);
}

char *apim_create() {
    char *msg = malloc(sizeof(char) * 1);
    msg[0] = '\0';

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

    log_debug("apim_read_param", "param bounds for arg %d is (%d-%d)", argc,
              end_pos, arg_start_pos);

    char *param =
        malloc(sizeof(char) * (end_pos - arg_start_pos - pipe_account) + 1);
    memset(param, '\0',
           sizeof(char) * (end_pos - arg_start_pos - pipe_account) + 1);

    for (int i = arg_start_pos; i < end_pos - pipe_account; i++) {
        param[i - arg_start_pos] = s[i];
    }

    log_debug("apim_read_param",
              "added %d amount of chars to arg %d with output of \"%s\"",
              end_pos - arg_start_pos - pipe_account, argc, param);

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
    log_debug("apim_parse_args", "parsing message \"%s\"", s);
    int argc = apim_count_args(s);

    char **parsed_args = malloc(sizeof(char *) * argc);

    for (int i = 0; i < argc; i++) {
        char *param = apim_read_param(s, i);
        int   k = strlen(param);
        log_debug("apim_parse_args", "parsing arg %d as: %s", i, param);
        log_debug("apim_parse_args", "arg %d is strlen of: %d", i, k);
        parsed_args[i] = param;
    }

    return parsed_args;
}

void apim_free_args(char **args, int argc) {
    log_debug("apim_free_args", "freeing %d args", argc);
    for (int i = 0; i < argc; i++) {
        log_debug("apim_free_args", "freeing \"%s\"", args[i]);
        free(args[i]);
    }
    log_debug("apim_free_args", "freed args, now freeing main pointer");
    free(args);
}

void apim_finish(char **s) {
    log_debug("apim_finish", "finishing api msg '%s'", *s);
    size_t len = strlen(*s);
    int    length = snprintf(NULL, 0, "%d", (int)len);
    char   str_len[length];
    sprintf(str_len, "%lu", len);
    log_debug("apim_finish",
              "count info:\nlen = %lu\nlength = %d\nstr_len = %s", len, length,
              str_len);
    char *finished_msg = malloc(sizeof(char) * 1);
    finished_msg[0] = '\0';
    utils_append(&finished_msg, str_len);
    utils_append(&finished_msg, *s);
    *s = finished_msg;
    log_debug("apim_finish", "finished api msg is: '%s'", *s);
}

void apim_capture_socket_msg(SSL *ssl, int fd, char **m_out) {
    char msg[99999];
    // int  n = recv(fd, msg, sizeof(msg), 0);
    // int n = SSL_read(ssl, msg, strlen(msg));
    int n = ssl_block_read(ssl, fd, msg, sizeof(msg));
    log_debug("apim_capture_socket_msg", "n value on RECV: %d", n);
    if (n < 0) {
        log_debug("apim_capture_socket_msg",
                  "read failed, abort connection suggested. msg read as: %s",
                  msg);
        *m_out = malloc(sizeof(char) * 1);
        (*m_out)[0] = '\0';
        return;
    }

    log_debug("apim_capture_socket_msg", "captured message: %s", msg);

    int first_non_digit = 0;
    while (first_non_digit < 5 && isdigit(msg[first_non_digit]) != 0)
        first_non_digit++;
    char api_msg_len[first_non_digit + 1];
    api_msg_len[first_non_digit] = '\0';
    memcpy(api_msg_len, &msg[0], first_non_digit);
    log_debug("apim_capture_socket_msg",
              "capture stats:\nfnd: %d\napi_msg_len: %s", first_non_digit,
              api_msg_len);

    *m_out = malloc(sizeof(char) * (atoi(api_msg_len) + 5));
    memcpy(*m_out, &msg[first_non_digit], atoi(api_msg_len));
    (*m_out)[atoi(api_msg_len)] = '\0';

    log_debug("apim_capture_socket_msg", "completed message is: %s", *m_out);
}