#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include "mbedtls/timing.h"
#include "psa/crypto.h"

typedef struct {
    int fd;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
} udp_ctx_t;

static int my_send(void *ctx, const unsigned char *buf, size_t len)
{
    printf("entrei no send\n");

    udp_ctx_t *c = (udp_ctx_t *) ctx;
    ssize_t ret = sendto(
        c->fd,
        buf, len,
        0,
        (struct sockaddr *) &c->server_addr,
        c->server_addr_len
    );

    if(ret >= 0)
        return (int) ret;

    if(errno == EAGAIN || errno == EWOULDBLOCK)
        return MBEDTLS_ERR_SSL_WANT_WRITE;

    return -1;
}

static int my_recv(void *ctx, unsigned char *buf, size_t len)
{
    printf("entrei no recv\n");

    udp_ctx_t *c = (udp_ctx_t *) ctx;
    ssize_t ret = recvfrom(
        c->fd,
        buf, len,
        0,
        (struct sockaddr *) &c->server_addr,
        &c->server_addr_len
    );

    if(ret > 0)
        return (int) ret;

    if(ret == 0)
        return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;

    if(errno == EAGAIN || errno == EWOULDBLOCK)
        return MBEDTLS_ERR_SSL_WANT_READ;

    return -1;
}

int main()
{
    psa_status_t status = psa_crypto_init();
    if(status != PSA_SUCCESS)
    {
        printf("psa_crypto_init failed: %d\n", (int) status);
        return 1;
    }

    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_timing_delay_context timer;

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);

    int ret = mbedtls_ssl_config_defaults(
        &conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_DATAGRAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_read_timeout(&conf, 1000);

    ret = mbedtls_ssl_setup(&ssl, &conf);
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("ssl_setup failed: %d (%s)\n", ret, errbuf);
        return 1;
    }

    mbedtls_ssl_set_timer_cb(
        &ssl,
        &timer,
        mbedtls_timing_set_delay,
        mbedtls_timing_get_delay
    );

    udp_ctx_t ctx;
    ctx.fd = socket(AF_INET, SOCK_DGRAM, 0);
    ctx.server_addr_len = sizeof(ctx.server_addr);

    memset(&ctx.server_addr, 0, sizeof(ctx.server_addr));
    ctx.server_addr.sin_family = AF_INET;
    ctx.server_addr.sin_port = htons(4433);
    inet_pton(AF_INET, "127.0.0.1", &ctx.server_addr.sin_addr);

    mbedtls_ssl_set_bio(&ssl, &ctx, my_send, my_recv, NULL);

    printf("START HANDSHAKE\n");

    do
    {
        ret = mbedtls_ssl_handshake(&ssl);
    }
    while(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("handshake failed: %d (%s)\n", ret, errbuf);
        close(ctx.fd);
        return 1;
    }

    printf("Ciphersuite: %s\n", mbedtls_ssl_get_ciphersuite(&ssl));

    printf("WRITE\n");

    const char *msg = "Ola Mano\n";

    do
    {
        ret = mbedtls_ssl_write(&ssl, (const unsigned char *) msg, strlen(msg));
    }
    while(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    printf("write ret = %d\n", ret);

    mbedtls_ssl_close_notify(&ssl);

    close(ctx.fd);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ssl_free(&ssl);

    return 0;
}