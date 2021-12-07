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
#include "time.h"

static struct cmd_command_list cmdh_commands;

// hacky but will let us access the client that sent us the current message. It
// should be a non-issue as our server is single-threaded anyway. This variable
// is set first thing when entering a command execution.
static int cur_fd;

int cmdh_msg_global(char **args, int argc);
int cmdh_register(char **args, int argc);
int cmdh_login(char **args, int argc);
int cmdh_close(char **args, int argc);
int cmdh_logout(char **args, int argc);

int cmdh_msg_global(char **args, int argc) {
    if (argc != 2) return 0;
    if (ss_is_fd_logged_in(cur_fd) == 1) {
        char *api_msg = apim_create();
        apim_add_param(api_msg, "SERV_RESPONSE", 0);
        apim_add_param(api_msg, "You are not logged in.", 1);
        apim_finish(api_msg);
        send(cur_fd, api_msg, strlen(api_msg), 0);
        return 0;
    }
    // create and format the current datetime to send
    time_t cur_date;
    time(&cur_date);
    char date[sizeof "2021-07-07T08:08:09Z"];
    strftime(date, sizeof(date), "%Y-%m-%d %I:%M:%S", localtime(&cur_date));
    char *name = ss_get_username(cur_fd);

    // print to server for server viewing reference
    printf("%s %s (%d): %s\n", date, name, cur_fd, args[1]);

    // format the message into `out`
    char out[4096];
    sprintf(out, "%s %s: %s\n", date, name, args[1]);

    // send the formatted message across all clients as this is the global chat
    char *api_msg = apim_create();
    apim_add_param(api_msg, "GLOBAL", 0);
    apim_add_param(api_msg, out, 1);
    apim_finish(api_msg);
    sm_propogate_message(cur_fd, api_msg);

    char *username = ss_get_username(cur_fd);
    int   uid = su_get_uid(username);
    sm_global_save_message(args[1], uid, date);

    // cleanups here
    free(api_msg);
    return 0;
}

int cmdh_msg_pm(char **args, int argc) {
    if (argc != 3) return 0;
    return 0;
}

int cmdh_register(char **args, int argc) {
    if (argc != 3) return 0;
    log_debug("cmdh_register", "size of password: %d", sizeof(args[2]));
    int   response = su_register_user(args[1], args[2]);
    char *api_msg = apim_create();
    apim_add_param(api_msg, "SERV_RESPONSE", 0);
    log_debug("cmdh_register", "received DB response of %d", response);
    if (response == SU_INVALID) {
        apim_add_param(api_msg, "The credentials provided are invalid.", 1);
    }
    if (response == SU_EXISTS) {
        apim_add_param(api_msg, "This user is already registered", 1);
    }
    if (response == 0) {
        apim_add_param(api_msg, "Register successful!", 1);
    }

    apim_finish(api_msg);
    send(cur_fd, api_msg, strlen(api_msg), 0);

    if (response == 0) {
        cmdh_login(args, argc);  // chain to a login
    }
    return 0;
}

int cmdh_login(char **args, int argc) {
    if (argc != 3) return 0;
    log_debug("cmdh_login", "do login with \"%s\" and \"%s\"", args[1],
              args[2]);
    int   response = su_validate_login(args[1], args[2]);
    char *api_msg = apim_create();
    apim_add_param(api_msg, "SERV_RESPONSE", 0);

    if (ss_is_user_logged_in(args[1]) == 0) {
        apim_add_param(api_msg,
                       "Already logged in, logout of other account first.", 1);
        apim_finish(api_msg);
        send(cur_fd, api_msg, strlen(api_msg), 0);
        free(api_msg);
        return 0;
    }
    if (response == SU_INVALID_LOGIN) {
        apim_add_param(api_msg, "Invalid password", 1);
    }
    if (response == SU_DNE) {
        apim_add_param(api_msg, "Cannot find account", 1);
    }
    if (response == 0) {
        ss_login_fd(cur_fd, args[1]);
        apim_add_param(api_msg, "Logged in!", 1);
    }
    apim_finish(api_msg);
    send(cur_fd, api_msg, strlen(api_msg), 0);
    free(api_msg);
    return 0;
}

int cmdh_close(char **args, int argc) {
    log_debug("cmdh_close", "user %d logged out, closing...", cur_fd);
    ss_logout_fd(cur_fd);
    ss_remove_child_connection(cur_fd);
    return 0;
}

int cmdh_logout(char **args, int argc) {
    char *api_msg = apim_create();
    apim_add_param(api_msg, "SERV_RESPONSE", 0);
    if (ss_is_fd_logged_in(cur_fd) == 1) {
        apim_add_param(api_msg, "You are not logged in.", 1);
    } else {
        ss_logout_fd(cur_fd);
        apim_add_param(api_msg, "Successfully logged you out.", 1);
    }
    apim_finish(api_msg);
    send(cur_fd, api_msg, strlen(api_msg), 0);
    return 0;
}

int cmdh_get_active_users(char **args, int argc) {
    log_debug("cmdh_get_active_users", "got active users:\n%s",
              ss_get_active_user_list());

    char *api_msg = apim_create();
    apim_add_param(api_msg, "SERV_RESPONSE", 0);
    if (ss_is_fd_logged_in(cur_fd) == 1) {
        apim_add_param(api_msg, "You are not logged in.", 1);
    } else if (ss_get_active_size() == 0) {
        apim_add_param(api_msg, "No users currently logged in.", 1);
    } else {
        apim_add_param(api_msg, ss_get_active_user_list(), 1);
    }

    apim_finish(api_msg);
    send(cur_fd, api_msg, strlen(api_msg), 0);
    return 0;
}

int cmdh_get_global_history(char **args, int argc) {
    log_debug("cmdh_get_global_history", "doing history request with: %d",
              atoi(args[1]));
    char *msg = apim_create();
    apim_add_param(msg, "SERV_RESPONSE", 0);
    if (argc != 2 || atoi(args[1]) > 15) {
        apim_add_param(msg, "Bad Request", 1);
        apim_finish(msg);
        send(cur_fd, msg, strlen(msg), 0);
        return 0;
    }

    sqlite3_stmt *query;

    int sql_err = sqlite3_prepare_v2(db_conn,
                                     "SELECT username, message, created_at \
        FROM message LEFT JOIN user \
        WHERE message.from_user == user.uid \
        ORDER BY id \
        DESC LIMIT 10",
                                     -1, &query, NULL);
    if (sql_err != SQLITE_OK) {
        log_debug("sm_global_create_history_stepper",
                  "error occured: ", sql_err);
        return 0;
    }

    // sqlite3_bind_int(query, 1, atoi(args[1]));

    for (;;) {
        int step_response = sqlite3_step(query);
        if (step_response == SQLITE_DONE) {
            log_debug("cmdh_get_global_history", "step as done is done");
            break;
        }
        if (step_response != SQLITE_ROW) {
            log_debug("cmdh_get_global_history", "step as row is done");
            log_debug("cmdh_get_global_history", "error occured: %s",
                      sqlite3_errmsg(db_conn));
            break;
        }
        // format the message into `out`
        char out[4096];
        sprintf(out, "%s %s: %s\n", sqlite3_column_text(query, 2),
                sqlite3_column_text(query, 0), sqlite3_column_text(query, 1));
        log_debug("cmdh_get_global_history", "message is: %s", out);
        char *msg = apim_create();
        apim_add_param(msg, "SERV_RESPONSE", 0);
        apim_add_param(msg, out, 1);
        apim_finish(msg);
        send(cur_fd, msg, strlen(msg), 0);
        free(msg);
    }
    sqlite3_finalize(query);
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
}

int cmdh_execute_command(char *command, int from_fd) {
    cur_fd = from_fd;

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