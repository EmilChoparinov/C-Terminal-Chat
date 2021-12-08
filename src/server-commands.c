#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api-messages.h"
#include "commands.h"
#include "logger.h"
#include "server-db.h"
#include "server-messages.h"
#include "server-state.h"
#include "server-users.h"
#include "sql-inject-checker.h"
#include "ssl-nonblock.h"
#include "time.h"
#include "utils.h"

static struct cmd_command_list cmdh_commands;

// hacky but will let us access the client that sent us the current message. It
// should be a non-issue as our server is single-threaded anyway. This variable
// is set first thing when entering a command execution.
static int  cur_fd;
static SSL *cur_ssl;

int cmdh_msg_global(char **args, int argc);
int cmdh_register(char **args, int argc);
int cmdh_login(char **args, int argc);
int cmdh_close(char **args, int argc);
int cmdh_logout(char **args, int argc);

/**
 * @brief Callback for processing a message meant for the global chatroom
 *
 * REQUIRES:
 *      - user to be logged in
 *      - contains 2 arguments. [commandname, message]
 *
 * @param args [commandname, message]
 * @param argc 2
 * @return int
 */
int cmdh_msg_global(char **args, int argc) {
    if (argc != 2) return 0;
    if (ss_is_fd_logged_in(cur_fd) == 1) {
        char *api_msg = apim_create();
        apim_add_param(&api_msg, "SERV_RESPONSE", 0);
        apim_add_param(&api_msg, "You are not logged in.", 1);
        apim_finish(&api_msg);
        ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
        return 0;
    }

    // create and format the current datetime to send
    time_t cur_date;
    time(&cur_date);
    char date[sizeof "2021-07-07T08:08:09Z"];
    strftime(date, sizeof(date), "%Y-%m-%d %I:%M:%S", localtime(&cur_date));
    char **name = ss_get_username(cur_fd);

    // print to server for server viewing reference
    printf("%s %s (%d): %s\n", date, *name, cur_fd, args[1]);

    // format the message into `out`
    char *out = malloc(sizeof(char) * 1);
    out[0] = '\0';
    utils_append(&out, date);
    utils_append(&out, " ");
    utils_append(&out, *name);
    utils_append(&out, ": ");
    utils_append(&out, args[1]);

    // send the formatted message across all clients as this is the global chat
    char *api_msg = apim_create();
    apim_add_param(&api_msg, "GLOBAL", 0);
    apim_add_param(&api_msg, out, 1);
    apim_finish(&api_msg);
    sm_propogate_message(cur_fd, api_msg);

    int uid = su_get_uid(*name);
    sm_global_save_message(args[1], uid, date);

    // cleanups here
    free(api_msg);
    free(out);
    return 0;
}

/**
 * @brief Callback for processing private messages
 *
 * REQUIRES:
 *      - user must to be logged in
 *      - contains 3 arguments. [commandname, usertosend, message]
 *      - recipient must be logged in
 *      - message must not be empty
 *      - cant talk to yourself
 *
 * @param args [commandname, usertosend, message]
 * @param argc 3
 * @return int
 */
int cmdh_msg_pm(char **args, int argc) {
    char *resp = apim_create();
    apim_add_param(&resp, "SERV_RESPONSE", 0);

    if (ss_is_fd_logged_in(cur_fd) == 1) {
        apim_add_param(&resp, "You are not logged in.", 1);
        apim_finish(&resp);
        ssl_block_write(cur_ssl, cur_fd, resp, strlen(resp));
        return 0;
    }
    if (argc != 3) {
        apim_add_param(&resp, "Bad Request", 1);
        apim_finish(&resp);
        ssl_block_write(cur_ssl, cur_fd, resp, strlen(resp));
        free(resp);
        return 0;
    }
    if (ss_is_user_logged_in(args[1]) != 0) {
        apim_add_param(
            &resp,
            "Cannot find user, you can only DM when both users are online.", 1);
        apim_finish(&resp);
        ssl_block_write(cur_ssl, cur_fd, resp, strlen(resp));
        free(resp);
        return 0;
    }
    if (strlen(args[2]) == 0) {
        apim_add_param(&resp, "Cannot send empty messages.", 1);
        apim_finish(&resp);
        ssl_block_write(cur_ssl, cur_fd, resp, strlen(resp));
        free(resp);
        return 0;
    }

    // create and format the current datetime to send
    time_t cur_date;
    time(&cur_date);
    char date[sizeof "2021-07-07T08:08:09Z"];
    strftime(date, sizeof(date), "%Y-%m-%d %I:%M:%S", localtime(&cur_date));
    char **name = ss_get_username(cur_fd);
    int    from_uid = su_get_uid(*name);

    // print to server for server viewing reference
    printf("%s %s to %s (%d): %s\n", date, *name, args[1], cur_fd, args[2]);

    // format the message into `out`
    char *out = malloc(sizeof(char) * 1);
    out[0] = '\0';
    utils_append(&out, date);
    utils_append(&out, " ");
    utils_append(&out, *name);
    utils_append(&out, " to ");
    utils_append(&out, args[1]);
    utils_append(&out, ": ");
    utils_append(&out, args[2]);

    int pm_fd = ss_get_fd_from_username(args[1]);
    if (pm_fd == -1) {
        apim_add_param(
            &resp,
            "Cannot find user, you can only DM when both users are online.", 1);
        apim_finish(&resp);
        ssl_block_write(cur_ssl, cur_fd, resp, strlen(resp));
        free(resp);
        return 0;
    }

    if (pm_fd == cur_fd) {
        apim_add_param(&resp,
                       "No friends to talk to? Damn that sucks here's my "
                       "whatsapp (+31 6 19347823).",
                       1);
        apim_finish(&resp);
        ssl_block_write(cur_ssl, cur_fd, resp, strlen(resp));
        free(resp);
        return 0;
    }

    apim_add_param(&resp, out, 1);
    apim_finish(&resp);
    int pm_fd_loc = ss_get_fd_loc(pm_fd);
    int recv_uid = su_get_uid(args[1]);

    ssl_block_write(ss_state->ssl_fd[pm_fd_loc], pm_fd, resp, strlen(resp));
    sm_pm_save_message(args[2], from_uid, recv_uid, date);
    free(resp);
    free(out);
    return 0;
}

/**
 * @brief Callback for processing a new user account
 *
 * REQUIRES:
 *      - correct argument count: [commandname, username, password]
 *      - user to not already exist
 *      - credentials are valid
 *      - username < 20 characters
 *
 * @param args [commandname, username, password]
 * @param argc 3
 * @return int
 */
int cmdh_register(char **args, int argc) {
    char *api_msg = apim_create();
    apim_add_param(&api_msg, "SERV_RESPONSE", 0);
    if (argc != 3) {
        apim_add_param(&api_msg, "Bad Request.", 1);
        apim_finish(&api_msg);
        ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
        free(api_msg);
        return 0;
    }
    log_debug("cmdh_register", "checking lengths user: %d password: %d",
              strlen(args[1]), strlen(args[2]));
    if (strlen(args[1]) > 20) {
        apim_add_param(&api_msg, "Username must be less than 20.", 1);
        apim_finish(&api_msg);
        ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
        free(api_msg);
        return 0;
    }

    log_debug("cmdh_register", "size of password: %d", sizeof(args[2]));
    int response = su_register_user(args[1], args[2]);
    log_debug("cmdh_register", "received DB response of %d", response);
    if (response == SU_INVALID) {
        apim_add_param(&api_msg, "The credentials provided are invalid.", 1);
    }
    if (response == SU_EXISTS) {
        apim_add_param(&api_msg, "This user is already registered", 1);
    }
    if (response == 0) {
        apim_add_param(&api_msg, "Register successful!", 1);
    }

    apim_finish(&api_msg);
    ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
    free(api_msg);

    if (response == 0) {
        cmdh_login(args, argc);  // chain to a login
    }
    return 0;
}

/**
 * @brief Callback to login a given user
 *
 * REQUIRES:
 *      - correct arguments. [commandname, username, password]
 *      - correct password
 *      - correct username
 *      - user is not already logged in somewhere else
 *      - account exists
 *
 * @param args [commandname, username, password]
 * @param argc 3
 * @return int
 */
int cmdh_login(char **args, int argc) {
    if (argc != 3) return 0;
    log_debug("cmdh_login", "do login with \"%s\" and \"%s\"", args[1],
              args[2]);
    int   response = su_validate_login(args[1], args[2]);
    char *api_msg = apim_create();
    apim_add_param(&api_msg, "SERV_RESPONSE", 0);

    if (ss_is_user_logged_in(args[1]) == 0) {
        apim_add_param(&api_msg,
                       "Already logged in, logout of other account first.", 1);
        apim_finish(&api_msg);
        ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
        free(api_msg);
        return 0;
    }
    if (response == SU_INVALID_LOGIN) {
        apim_add_param(&api_msg, "Invalid password", 1);
    }
    if (response == SU_DNE) {
        apim_add_param(&api_msg, "Cannot find account", 1);
    }
    if (response == 0) {
        ss_login_fd(cur_fd, args[1]);
        apim_add_param(&api_msg, "Logged in!", 1);
    }
    apim_finish(&api_msg);
    ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
    free(api_msg);
    return 0;
}

/**
 * @brief Callback to close a user connection (they ctrl+c'd or something). It
 * first logs out and then frees the socket connection
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdh_close(char **args, int argc) {
    log_debug("cmdh_close", "user %d logged out, closing...", cur_fd);
    ss_logout_fd(cur_fd);
    ss_remove_child_connection(cur_fd);
    return 0;
}

/**
 * @brief Callback for logging out a user
 *
 * REQUIRES:
 *      - user to be logged in
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdh_logout(char **args, int argc) {
    char *api_msg = apim_create();
    apim_add_param(&api_msg, "SERV_RESPONSE", 0);
    if (ss_is_fd_logged_in(cur_fd) == 1) {
        apim_add_param(&api_msg, "You are not logged in.", 1);
    } else {
        ss_logout_fd(cur_fd);
        apim_add_param(&api_msg, "Successfully logged you out.", 1);
    }
    apim_finish(&api_msg);
    ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
    return 0;
}

/**
 * @brief Callback for getting a list of active users
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdh_get_active_users(char **args, int argc) {
    log_debug("cmdh_get_active_users", "got active users:\n%s",
              ss_get_active_user_list());

    char *api_msg = apim_create();
    apim_add_param(&api_msg, "SERV_RESPONSE", 0);
    if (ss_is_fd_logged_in(cur_fd) == 1) {
        apim_add_param(&api_msg, "You are not logged in.", 1);
    } else if (ss_get_active_size() == 0) {
        apim_add_param(&api_msg, "No users currently logged in.", 1);
    } else {
        apim_add_param(&api_msg, ss_get_active_user_list(), 1);
    }

    apim_finish(&api_msg);
    ssl_block_write(cur_ssl, cur_fd, api_msg, strlen(api_msg));
    return 0;
}

/**
 * @brief Callback for getting the global history
 *
 * REQUIRES:
 *      - arg count is correct. [commandname, username]
 *      - request asks for no more than 30 messages
 *
 * @param args [commandname, username]
 * @param argc 2
 * @return int
 */
int cmdh_get_global_history(char **args, int argc) {
    log_debug("cmdh_get_global_history", "doing history request with: %d",
              atoi(args[1]));
    char *msg = apim_create();
    apim_add_param(&msg, "SERV_RESPONSE", 0);
    if (argc != 2 || atoi(args[1]) > 30) {
        apim_add_param(&msg, "Bad Request, request count may be too high.", 1);
        apim_finish(&msg);
        ssl_block_write(cur_ssl, cur_fd, msg, strlen(msg));
        return 0;
    }

    sqlite3_stmt *query;

    int sql_err =
        sqlite3_prepare_v2(db_conn,
                           "SELECT username, message, created_at FROM ( \
                                SELECT * FROM message LEFT JOIN user \
                                WHERE message.from_user == user.uid \
                                AND message.is_dm == 1 \
                                ORDER BY id \
                                DESC LIMIT ? \
                            ) ORDER BY id ASC",
                           -1, &query, NULL);
    if (sql_err != SQLITE_OK) {
        log_debug("sm_global_create_history_stepper",
                  "error occured: ", sql_err);
        return 0;
    }

    sqlite3_bind_int(query, 1, atoi(args[1]));
    char *resp_str = malloc(sizeof(char) * 1);
    resp_str[0] = '\0';

    for (;;) {
        int step_resp = sqlite3_step(query);
        if (step_resp == SQLITE_DONE) {
            log_debug("cmdh_get_global_history", "query completed");
            break;
        }
        if (step_resp != SQLITE_ROW) {
            log_debug("cmdh_get_global_history",
                      "step as row failed. error occured: %s",
                      sqlite3_errmsg(db_conn));
            break;
        }

        char *out = malloc(sizeof(char) * 1);
        out[0] = '\0';
        utils_append(&out, (char *)sqlite3_column_text(query, 2));
        utils_append(&out, " ");
        utils_append(&out, (char *)sqlite3_column_text(query, 0));
        utils_append(&out, ": ");
        utils_append(&out, (char *)sqlite3_column_text(query, 1));
        utils_append(&out, "\n");
        utils_append(&resp_str, out);
        free(out);
    }
    apim_add_param(&msg, resp_str, 1);
    apim_finish(&msg);
    ssl_block_write(cur_ssl, cur_fd, msg, strlen(msg));
    free(resp_str);
    free(msg);

    sqlite3_finalize(query);
    return 0;
}

/**
 * @brief Callback for getting private message history
 *
 * REQUIRES:
 *      - requester to be within the chat system
 *      - recepient is online
 *      - arg count is correct. [commandname, recepient, message]
 *      - valid recepient
 *
 * @param args [commandname, recepient, message]
 * @param argc 3
 * @return int
 */
int cmdh_pm_history(char **args, int argc) {
    char *msg = apim_create();
    apim_add_param(&msg, "SERV_RESPONSE", 0);
    if (argc != 3 || atoi(args[2]) > 30) {
        apim_add_param(&msg, "Bad Request, request count may be too high.", 1);
        apim_finish(&msg);
        ssl_block_write(cur_ssl, cur_fd, msg, strlen(msg));
        return 0;
    }
    if (ss_is_fd_logged_in(cur_fd) == 1) {
        apim_add_param(&msg, "You are not logged in.", 1);
        apim_finish(&msg);
        ssl_block_write(cur_ssl, cur_fd, msg, strlen(msg));
        return 0;
    }

    sqlite3_stmt *query;

    int sql_err = sqlite3_prepare_v2(
        db_conn,
        "SELECT message, created_at, \"from\", \"to\" FROM ( \
            SELECT \
	        message.id, message, created_at, \
	        user_from.username AS \"from\", user_to.username as \"to\" \
	        FROM message \
            LEFT JOIN user user_from \
            ON message.from_user == user_from.uid \
            LEFT JOIN user user_to \
            ON message.to_user == user_to.uid \
            WHERE message.is_dm == 0 \
            AND (message.from_user == ? OR message.from_user == ?) \
            AND (message.to_user == ? OR message.to_user == ?) \
            ORDER BY message.id DESC \
            LIMIT ? \
        ) ORDER BY id ASC",
        -1, &query, NULL);
    if (sql_err != SQLITE_OK) {
        log_debug("sm_global_create_history_stepper",
                  "error occured: ", sql_err);
        return 0;
    }

    char **name = ss_get_username(cur_fd);
    int    from_user = su_get_uid(*name);
    int    to_user = su_get_uid(args[1]);
    if (from_user < 0 || to_user < 0) {
        apim_add_param(&msg, "Name given was invalid.", 1);
        apim_finish(&msg);
        ssl_block_write(cur_ssl, cur_fd, msg, strlen(msg));
        return 0;
    }

    sqlite3_bind_int(query, 1, from_user);
    sqlite3_bind_int(query, 2, to_user);
    sqlite3_bind_int(query, 3, from_user);
    sqlite3_bind_int(query, 4, to_user);
    sqlite3_bind_int(query, 5, atoi(args[2]));
    char *resp_str = malloc(sizeof(char) * 1);
    resp_str[0] = '\0';

    int has_data = 1;
    for (;;) {
        int step_resp = sqlite3_step(query);
        if (step_resp == SQLITE_DONE) {
            log_debug("cmdh_pm_history", "query completed");
            break;
        }
        if (step_resp != SQLITE_ROW) {
            log_debug("cmdh_pm_history",
                      "step as row failed. error occured: %s",
                      sqlite3_errmsg(db_conn));
            break;
        }

        has_data = 0;
        char *out = malloc(sizeof(char) * 1);
        out[0] = '\0';
        utils_append(&out, (char *)sqlite3_column_text(query, 1));
        utils_append(&out, " ");
        utils_append(&out, (char *)sqlite3_column_text(query, 2));
        utils_append(&out, " to ");
        utils_append(&out, (char *)sqlite3_column_text(query, 3));
        utils_append(&out, ": ");
        utils_append(&out, (char *)sqlite3_column_text(query, 0));
        utils_append(&out, "\n");
        utils_append(&resp_str, out);
        free(out);
    }

    if (has_data == 1) {
        utils_append(&resp_str, "No private message history.");
    }
    apim_add_param(&msg, resp_str, 1);
    apim_finish(&msg);
    ssl_block_write(cur_ssl, cur_fd, msg, strlen(msg));
    free(resp_str);
    free(msg);

    sqlite3_finalize(query);
    return 0;
}

/**
 * @brief health ping for the client, unused
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdh_server_health_ping(char **args, int argc) {
    log_debug("cmdh_server_health_ping",
              "client pinged server for connection test");
    return 0;
}

void cmdh_setup_server_commands() {
    cmd_create_command_list(&cmdh_commands);

    // register server commands/requests here
    cmd_register_command(&cmdh_commands, "GLOBAL", &cmdh_msg_global);
    cmd_register_command(&cmdh_commands, "CLOSE", &cmdh_close);
    cmd_register_command(&cmdh_commands, "REGISTER", &cmdh_register);
    cmd_register_command(&cmdh_commands, "LOGIN", &cmdh_login);
    cmd_register_command(&cmdh_commands, "LOGOUT", &cmdh_logout);
    cmd_register_command(&cmdh_commands, "USERS", &cmdh_get_active_users);
    cmd_register_command(&cmdh_commands, "HISTORY", &cmdh_get_global_history);
    cmd_register_command(&cmdh_commands, "PMHISTORY", &cmdh_pm_history);
    cmd_register_command(&cmdh_commands, "DM", &cmdh_msg_pm);
    cmd_register_command(&cmdh_commands, "HEALTH", &cmdh_server_health_ping);
}

int cmdh_execute_command(char *command, int from_fd) {
    cur_fd = from_fd;
    int fd_loc = ss_get_fd_loc(from_fd);
    cur_ssl = ss_state->ssl_fd[fd_loc];

    char **parsed_args = apim_parse_args(command);
    if (cmd_is_command(&cmdh_commands, parsed_args[0]) == 1) {
        printf("Invalid Server Command \"%s\"\n", command);
        return 0;
    }

    int result = cmd_execute(&cmdh_commands, parsed_args[0], parsed_args,
                             apim_count_args(command));

    apim_free_args(parsed_args, apim_count_args(command));

    return result;
}

void cmdh_free_server_commands() { cmd_deregister(&cmdh_commands); }