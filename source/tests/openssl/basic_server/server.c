#include <openssl/ssl.h>
#include <openssl/quic.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static char *server_cert_file = "./common/certs/server.crt";
static char *server_key_file = "./common/certs/server.key";

static const unsigned char alpn_ossltest[] = {
    0x08, 0x6f, 0x73, 0x73, 0x6c, 0x74, 0x65, 0x73, 0x74
};

static int select_alpn(SSL *ssl,
                       const unsigned char **out, unsigned char *out_len,
                       const unsigned char *in, unsigned int in_len,
                       void *arg)
{
    if (SSL_select_next_proto((unsigned char **)out, out_len,
                              alpn_ossltest, sizeof(alpn_ossltest),
                              in, in_len) != OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    return SSL_TLSEXT_ERR_OK;
}

int main(void)
{
    SSL_CTX *ctx = NULL;
    SSL *listener = NULL;
    int fd = -1;

    ctx = SSL_CTX_new(OSSL_QUIC_server_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (SSL_CTX_use_certificate_file(ctx, server_cert_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, server_key_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "private key does not match certificate\n");
        return 1;
    }

    SSL_CTX_set_alpn_select_cb(ctx, select_alpn, NULL);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4433);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    listener = SSL_new_listener(ctx, 0);
    if (listener == NULL) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (!SSL_set_fd(listener, fd)) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (!SSL_listen(listener)) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (!SSL_set_blocking_mode(listener, 1)) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    printf("QUIC server listening on UDP 4433\n");

    for (;;) {
        SSL *conn = SSL_accept_connection(listener, 0);
        size_t written = 0;

        if (conn == NULL) {
            ERR_print_errors_fp(stderr);
            continue;
        }

        printf("client connected\n");

        if (!SSL_write_ex2(conn, "hello\n", 6, SSL_WRITE_FLAG_CONCLUDE, &written)) {
            ERR_print_errors_fp(stderr);
            SSL_free(conn);
            continue;
        }


        while(1);

        if (SSL_shutdown(conn) != 1)
            ERR_print_errors_fp(stderr);

        SSL_free(conn);
    }

    SSL_free(listener);
    SSL_CTX_free(ctx);
    close(fd);
    return 0;
}