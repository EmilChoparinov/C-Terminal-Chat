
#include "server-users.h"

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdio.h>

#include "server-db.h"
#include "sql-inject-checker.h"

#define SU_EXISTS 2       /* User already exists */
#define SU_INVALID 3      /* Data is invalid in some manner */
#define SU_LOGIN_EXISTS 4 /* User is already logged in */

int su_register_user(char *username, char *password) {
    int user_ok = sic_is_sql_ok(username);
    int password_ok = sic_is_sql_ok(password);

    if (user_ok != 0 || password_ok != 0) {
        return SU_INVALID;
    }

    if (su_has_user(username) == 1) {
        return SU_EXISTS;
    }

    unsigned char salt[32];  // 32 is just an example
    RAND_bytes(salt, 32);

    SHA256_CTX    ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    // SHA256_Update(&ctx, (unsigned char *)args[2], strlen(args[2]));
    SHA256_Final(md, &ctx);

    sqlite3_stmt *query;

    int err = sqlite3_prepare_v2(
        db_conn, "INSERT INTO user(username, password, salt) VALUES(?,?,?)", -1,
        &query, NULL);
    sqlite3_bind_text(query, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(query, 2, password, -1, SQLITE_TRANSIENT);

    char out[4096];
    sprintf(out,
            "INSERT INTO user(username, password, salt) VALUES (\"%s\", "
            "\"%s\", 69)",
            username, password);

    int resp = sdb_execute(1, out, NULL);
    if (resp != SQLITE_OK) {
        return SU_INVALID;
    }
    return 0;
}

void su_validate_login(char *username, char *password,
                       void (*response)(int res)) {}

static int su_cb_has_user(void *data, int argc, char **argv, char **col_name) {
    if (argc > 0) return 1;
    return 0;
}

int su_has_user(char *username) {
    char out[4096];
    sprintf(
        out,
        "SELECT EXISTS(SELECT 1 FROM user WHERE username == \"%s\" LIMIT 1)",
        username);
    return sdb_execute(0, out, &su_cb_has_user);
}