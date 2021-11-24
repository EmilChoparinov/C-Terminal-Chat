
#include "server-users.h"

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "server-db.h"
#include "sql-inject-checker.h"

int su_register_user(char *username, char *password) {
    log_debug("su_register_user", "entering register use for \"%s\"", username);
    int user_ok = sic_is_sql_ok(username);
    int password_ok = sic_is_sql_ok(password);

    if (user_ok != 0 || password_ok != 0) {
        return SU_INVALID;
    }

    if (su_has_user(username) == 1) {
        return SU_EXISTS;
    }

    log_debug("su_register_user", "creating salt");
    unsigned char salt[32];
    RAND_bytes(salt, 32);
    char salt_out[2 * 32 + 1];
    for (int i = 0; i < 32; i++) {
        sprintf(salt_out + (i * 2), "%02x", salt[i]);
    }
    salt_out[2 * 32] = '\0';
    log_debug("su_register_user", "salt is: %s", salt_out);

    char salted_pw[strlen(password) + 2 * 32 + 1];
    sprintf(salted_pw, "%s%s", password, salt_out);

    log_debug("su_register_user", "hashing password...");
    SHA256_CTX    ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char *)salted_pw, strlen(salted_pw));
    SHA256_Final(md, &ctx);
    log_debug("su_register_user", "hashing ended");

    log_debug("su_register_user", "preparing sql statement");
    sqlite3_stmt *query;

    int err = sqlite3_prepare_v2(
        db_conn, "INSERT INTO user(username, password, salt) VALUES(?,?,?)", -1,
        &query, NULL);
    if (err != SQLITE_OK) {
        log_debug("su_register_user", "SQL Error Occured: Code %s", err);
        return 0;
    }

    int  size = SHA256_DIGEST_LENGTH;
    char out_md[2 * size + 1];
    for (int i = 0; i < size; i++) {
        sprintf(out_md + (i * 2), "%02x", md[i]);
    }
    out_md[2 * size] = '\0';

    sqlite3_bind_text(query, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(query, 2, out_md, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(query, 3, salt_out, -1, SQLITE_TRANSIENT);

    int resp = sqlite3_step(query);
    sqlite3_finalize(query);

    if (resp == SQLITE_DONE) {
        return 0;
    }
    return 1;
}

int su_validate_login(char *username, char *password) {
    // check if user exists
    if (su_has_user(username) == 0) {
        return SU_DNE;
    }

    // get salt from DB
    sqlite3_stmt *query;

    int sql_err = sqlite3_prepare_v2(
        db_conn, "SELECT salt,password FROM user where username == ? LIMIT 1;",
        -1, &query, NULL);
    if (sql_err != SQLITE_OK) {
        log_debug("su_validate_login", "error with SQL");
    }
    sqlite3_bind_text(query, 1, username, -1, SQLITE_TRANSIENT);
    int resp = sqlite3_step(query);
    if (resp != SQLITE_ROW) {
        return SU_DNE;
    }
    log_debug("su_validate_login", "getting salt from col");
    char *salt = (char *)sqlite3_column_text(query, 0);
    char *db_password = (char *)sqlite3_column_text(query, 1);

    char salted_pw[strlen(salt) + strlen(password)];
    sprintf(salted_pw, "%s%s", password, salt);

    SHA256_CTX    ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char *)salted_pw, strlen(salted_pw));
    SHA256_Final(md, &ctx);

    int  size = SHA256_DIGEST_LENGTH;
    char out_md[2 * size + 1];
    for (int i = 0; i < size; i++) {
        sprintf(out_md + (i * 2), "%02x", md[i]);
    }
    out_md[2 * size] = '\0';

    log_debug("su_validate_login", "compare:\n%s\n%s", out_md, db_password);

    int is_valid_login = strcmp(out_md, db_password);
    sqlite3_finalize(query);
    return is_valid_login;
}

int su_has_user(char *username) {
    sqlite3_stmt *query;

    int err = sqlite3_prepare_v2(
        db_conn,
        "SELECT EXISTS(SELECT 1 FROM user WHERE username == ? LIMIT 1)", -1,
        &query, NULL);
    sqlite3_bind_text(query, 1, username, -1, SQLITE_TRANSIENT);

    if (err != SQLITE_OK) {
        log_debug("su_has_user", "error constructing has_user query");
        return 0;
    }

    int resp = sqlite3_step(query);
    if (resp == SQLITE_ROW) {
        int val = (int)sqlite3_column_int(query, 0);
        log_debug("su_has_user", "recieved value: %d", val);
        return val;
    }
    return 0;
}