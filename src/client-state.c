#include "client-state.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

void cs_reset() {
    memset(&cs_state, 0, sizeof(cs_state));
    cs_state.connection_fd = -1;
    cs_state.connected = -1;

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL     *ssl = SSL_new(ctx);

    cs_state.ssl_ctx = ctx;
    cs_state.ssl_fd = ssl;
}

void cs_free() {
    SSL_free(cs_state.ssl_fd);
    SSL_CTX_free(cs_state.ssl_ctx);
}