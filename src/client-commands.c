#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api-messages.h"
#include "client-state.h"
#include "commands.h"
#include "logger.h"
#include "ssl-nonblock.h"
#include "utils.h"

static struct cmd_command_list cmdc_commands;

/**
 * @brief Exit command will close the program
 *
 * @param args no args
 * @param argc no args
 * @return int
 */
int cmdc_exit(char **args, int argc) {
    log_debug("cmdc_exit", "logging out of %d", cs_state.connection_fd);

    char *msg = apim_create();
    apim_add_param(&msg, "CLOSE", 0);
    apim_finish(&msg);
    // send(cs_state.connection_fd, msg, strlen(msg), 0);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, msg, strlen(msg));

    close(cs_state.connection_fd);
    free(msg);
    return 1;
}

/**
 * @brief Login command will initiate a login request with the server
 *
 * SECURITY FEATURES:
 *      The password is hashed at the client level here. An attacker could not
 *      obtain the password through listening
 *
 * @param args [commandname, username, password]
 * @param argc 3
 * @return int
 */
int cmdc_login(char **args, int argc) {
    log_debug("cmdc_exit", "do login");

    if (argc != 3) {
        printf("User login command invalid, do /help to see usage\n");
        return 0;
    }

    SHA256_CTX    ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char *)args[2], strlen(args[2]));
    SHA256_Final(md, &ctx);

    int  size = SHA256_DIGEST_LENGTH;
    char out[2 * size + 1];
    for (int i = 0; i < size; i++) {
        sprintf(out + (i * 2), "%02x", md[i]);
    }
    out[2 * size] = '\0';

    log_debug("cmdc_login", "output hash is: %s", out);

    // send login request
    char *msg = apim_create();
    apim_add_param(&msg, "LOGIN", 0);
    apim_add_param(&msg, args[1], 1);
    apim_add_param(&msg, out, 2);
    apim_finish(&msg);
    // send(cs_state.connection_fd, msg, strlen(msg), 0);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, msg, strlen(msg));

    free(msg);

    return 0;
}

/**
 * @brief Logs client out of session
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdc_logout(char **args, int argc) {
    log_debug("cmdc_logout", "entered logout routine");

    char *api_msg = apim_create();
    apim_add_param(&api_msg, "LOGOUT", 0);
    apim_finish(&api_msg);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, api_msg,
                    strlen(api_msg));
    free(api_msg);
    return 0;
}

/**
 * @brief Register a new account
 *
 * SECURITY FEATURES:
 *      The password is hashed at the client level here. The password is
 *      protected from being listened to the network connection.
 *
 * @param args [commandname, username, password]
 * @param argc 3
 * @return int
 */
int cmdc_register(char **args, int argc) {
    log_debug("cmdc_register", "do register");

    if (argc != 3) {
        printf("User register command invalid, do /help to see usage\n");
        return 0;
    }

    // this is here because the server cannot check the password length as its
    // already hashed! If the client really wanted a weak password they totally
    // could
    if (strlen(args[2]) < 8) {
        printf("Password must be greater than 8.\n");
        return 0;
    }

    SHA256_CTX    ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char *)args[2], strlen(args[2]));
    SHA256_Final(md, &ctx);

    int  size = SHA256_DIGEST_LENGTH;
    char out[2 * size + 1];
    for (int i = 0; i < size; i++) {
        sprintf(out + (i * 2), "%02x", md[i]);
    }
    out[2 * size] = '\0';

    log_debug("cmdc_register", "output hash is: %s", out);

    // send register request
    char *msg = apim_create();
    apim_add_param(&msg, "REGISTER", 0);
    apim_add_param(&msg, args[1], 1);
    apim_add_param(&msg, out, 2);
    apim_finish(&msg);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, msg, strlen(msg));
    free(msg);

    return 0;
}

/**
 * @brief Send a request to print out users
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdc_users(char **args, int argc) {
    log_debug("cmdc_users", "listing users");

    char *msg = apim_create();
    apim_add_param(&msg, "USERS", 0);
    apim_finish(&msg);
    // send(cs_state.connection_fd, msg, strlen(msg), 0);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, msg, strlen(msg));
    free(msg);

    printf("Getting list of users:\n");

    return 0;
}

/**
 * @brief Print out the help text
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdc_help(char **args, int argc) {
    log_debug("cmdc_help", "doing help");

    printf(
        "The list of commands and their usages in this app:\n\n \
/login - Logs you into an existing account with this app\n \
            USAGE: /login [username] [password]\n \
/logout - Logs you out of an account currently in session\n \
            USAGE: /logout\n \
/register - Register a new account with a unique username\n \
            USAGE: /register [username] [password]\n \
/users - Display all online users\n \
            USAGE: /users\n \
/history - Get the history of the global channel (up to 30 messages)\n \
            USAGE: /history [message count]\n \
/pmhistory - Get your private message history from a user\n \
            USAGE: /pmhistory [user you messaged] [message count]\n \
/help - Help command\n \
            USAGE: /help\n \
/exit - Exit the chat and close the program\n \
            USAGE: /exit\n\n");

    return 0;
}

/**
 * @brief Printer for global messages being recieved from the server
 *
 * @param args [commandname, message]
 * @param argc 2
 * @return int
 */
int cmdc_global(char **args, int argc) {
    printf("%s\n", args[1]);
    return 0;
}

/**
 * @brief When you receive a messagee that you were disconnected, this will run
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdc_server_disconnected(char **args, int argc) {
    printf("Server has disconnected you.\n");
    return 1;
}

/**
 * @brief When you receive a generic server message
 *
 * @param args [commandname, message]
 * @param argc 2
 * @return int
 */
int cmdc_server_response(char **args, int argc) {
    log_debug("cmdc_server", "Receieved server response");
    if (argc > 0) {
        printf("%s\n", args[1]);
    }
    return 0;
}

/**
 * @brief Request the history of public chat messages (up to 30)
 *
 * @param args [commandname, amount]
 * @param argc 2
 * @return int
 */
int cmdc_history(char **args, int argc) {
    log_debug("cmdc_history", "Revieved history command");
    if (argc != 2) {
        printf("History command invalid, do /help to get command format.\n");
        return 0;
    }
    char *msg = apim_create();
    apim_add_param(&msg, "HISTORY", 0);
    apim_add_param(&msg, args[1], 1);
    apim_finish(&msg);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, msg, strlen(msg));
    free(msg);
    return 0;
}

/**
 * @brief Request the history of private chat messages (up to 30)
 *
 * @param args [commandname, username, amount]
 * @param argc 3
 * @return int
 */
int cmdc_pm_history(char **args, int argc) {
    log_debug("cmdc_pm_history", "Entered PM history");
    if (argc != 3) {
        printf("PM History command invalid, do /help to get command format\n");
        return 0;
    }
    char *msg = apim_create();
    apim_add_param(&msg, "PMHISTORY", 0);
    apim_add_param(&msg, args[1], 1);
    apim_add_param(&msg, args[2], 2);
    apim_finish(&msg);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, msg, strlen(msg));
    free(msg);
    return 0;
}

/**
 * @brief The server regularly pings to see if this socket is still active, this
 * is where it goes
 *
 * @param args none
 * @param argc none
 * @return int
 */
int cmdc_server_health_ping(char **args, int argc) {
    log_debug("cmdc_server_health_ping",
              "server pinged client for connection test");
    return 0;
}

/**
 * @brief Private DM command, this is run manually and not managed by the
 * command framework
 *
 * @param msg custom command message with format "@[user] [message]"
 * @return int
 */
int cmdc_private_dm(char *msg) {
    log_debug("cmdc_private_dm", "parsing message '%s'", msg);
    int idx = 1;
    while (idx < strlen(msg) && msg[idx] != ' ') {
        idx++;
    }
    int username_size = idx - 1;
    int dm_size = strlen(msg) - username_size;

    char username[username_size + 1];
    memcpy(username, &msg[1], username_size);
    username[username_size] = '\0';

    char dm[dm_size + 1];
    memcpy(dm, &msg[username_size + 2], dm_size);
    dm[dm_size] = '\0';

    log_debug("cmdc_private_dm", "sending message to '%s' with content '%s'",
              username, dm);

    char *api_msg = apim_create();
    apim_add_param(&api_msg, "DM", 0);
    apim_add_param(&api_msg, username, 1);
    apim_add_param(&api_msg, dm, 2);
    apim_finish(&api_msg);
    // send(cs_state.connection_fd, api_msg, strlen(api_msg), 0);
    ssl_block_write(cs_state.ssl_fd, cs_state.connection_fd, api_msg,
                    strlen(api_msg));
    free(api_msg);
    return 0;
}

void cmdc_setup_client_commands() {
    cmd_create_command_list(&cmdc_commands);

    /* CLIENT ONLY COMMANDS */
    cmd_register_command(&cmdc_commands, "/exit", &cmdc_exit);
    cmd_register_command(&cmdc_commands, "/login", &cmdc_login);
    cmd_register_command(&cmdc_commands, "/logout", &cmdc_logout);
    cmd_register_command(&cmdc_commands, "/register", &cmdc_register);
    cmd_register_command(&cmdc_commands, "/users", &cmdc_users);
    cmd_register_command(&cmdc_commands, "/help", &cmdc_help);
    cmd_register_command(&cmdc_commands, "/history", &cmdc_history);
    cmd_register_command(&cmdc_commands, "/pmhistory", &cmdc_pm_history);

    /* SERVER RECEIVED COMMANDS */
    cmd_register_command(&cmdc_commands, "GLOBAL", &cmdc_global);
    cmd_register_command(&cmdc_commands, "SERVER_DISCONNECTED",
                         &cmdc_server_disconnected);
    cmd_register_command(&cmdc_commands, "SERV_RESPONSE",
                         &cmdc_server_response);
    cmd_register_command(&cmdc_commands, "HEALTH", &cmdc_server_health_ping);
}

void cmdc_free_client_commands() { cmd_deregister(&cmdc_commands); }

int cmdc_execute_command(char *command) {
    log_debug("cmdc_execute_command", "executing command \"%s\"...", command);

    log_debug("cmdc_execute_command", "collections string arguments for \"%s\"",
              command);
    int    argc;
    char **args = utils_str_to_args(command, &argc);
    log_debug("cmdc_execute_command", "collected total %d arguments", argc);

    if (argc == 0) return 0;  // default empty to expected

    if (cmd_is_command(&cmdc_commands, args[0]) == 1) {
        printf("Invalid Command, do /help to view all commands\n");
        return 0;
    }

    int result = cmd_execute(&cmdc_commands, args[0], args, argc);

    // free arguments after use
    for (int i = 0; i < argc; i++) {
        free(args[i]);
    }
    free(args);

    return result;
}

int cmdc_execute_server_command(char *command) {
    log_debug("cmdc_execute_server_command", "executing command \"%s\"...",
              command);

    char **parsed_args = apim_parse_args(command);
    if (cmd_is_command(&cmdc_commands, parsed_args[0]) == 1) {
        log_debug("cmdc_execute_server_command",
                  "received invalid command from server %s", command);
        return 0;
    }

    int result = cmd_execute(&cmdc_commands, parsed_args[0], parsed_args,
                             apim_count_args(command));

    apim_free_args(parsed_args, apim_count_args(command));
    return result;
}