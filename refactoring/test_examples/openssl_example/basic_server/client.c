#include <openssl/ssl.h>
#include <openssl/quic.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char alpn_ossltest[] = {
    0x08, 0x6f, 0x73, 0x73, 0x6c, 0x74, 0x65, 0x73, 0x74
};

int main(void)
{
    SSL_CTX *ctx = NULL;
    SSL *conn = NULL;
    int fd = -1;
    unsigned char buf[1024];
    size_t bytes_read = 0;

    ctx = SSL_CTX_new(OSSL_QUIC_client_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    if (SSL_CTX_set_alpn_protos(ctx, alpn_ossltest, sizeof(alpn_ossltest)) != 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    conn = SSL_new(ctx);
    if (conn == NULL) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (!SSL_set_fd(conn, fd)) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    SSL_set_connect_state(conn);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4433);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    if (!SSL_connect(conn)) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    
    printf("Connected to QUIC server\n");
    
    if (!SSL_read_ex(conn, buf, sizeof(buf) - 1, &bytes_read)) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    
    buf[bytes_read] = '\0';
    printf("Received: %s", (char *)buf);
    
    SSL_shutdown(conn);
    SSL_free(conn);
    SSL_CTX_free(ctx);
    
    close(fd);
    return 0;
}