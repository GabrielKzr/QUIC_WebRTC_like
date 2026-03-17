#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "mbedtls/timing.h"
#include "psa/crypto.h"

typedef struct {
    int fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
} udp_ctx_t;

static int my_send(void *ctx, const unsigned char *buf, size_t len)
{
    printf("entrei no send\n");

    udp_ctx_t *c = (udp_ctx_t *) ctx;
    ssize_t ret = sendto(
        c->fd,
        buf, len,
        0,
        (struct sockaddr *) &c->client_addr,
        c->client_addr_len
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
        (struct sockaddr *) &c->client_addr,
        &c->client_addr_len
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
    mbedtls_pk_context pkey;
    mbedtls_x509_crt srvcert;
    mbedtls_timing_delay_context timer;
    mbedtls_ssl_cookie_ctx cookie_ctx;

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_pk_init(&pkey);
    mbedtls_x509_crt_init(&srvcert);
    mbedtls_ssl_cookie_init(&cookie_ctx);

    mbedtls_x509_crt_parse_file(&srvcert, "../../../../common/cert.pem");
    mbedtls_pk_parse_keyfile(&pkey, "../../../../common/key.pem", NULL);

    mbedtls_ssl_config_defaults(
        &conf,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_DATAGRAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );

    mbedtls_ssl_conf_read_timeout(&conf, 10000);
    mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey);

    // setup do cookie
    int ret = mbedtls_ssl_cookie_setup(&cookie_ctx);
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("cookie_setup failed: %d (%s)\n", ret, errbuf);
        return 1;
    }

    mbedtls_ssl_conf_dtls_cookies(
        &conf,
        mbedtls_ssl_cookie_write,
        mbedtls_ssl_cookie_check,
        &cookie_ctx
    );

    mbedtls_ssl_setup(&ssl, &conf);

    mbedtls_ssl_set_timer_cb(
        &ssl,
        &timer,
        mbedtls_timing_set_delay,
        mbedtls_timing_get_delay
    );

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4433);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(fd, (struct sockaddr *) &addr, sizeof(addr));

    printf("Server listening on port 4433 (UDP/DTLS)...\n");

reset:
    mbedtls_ssl_session_reset(&ssl);

    udp_ctx_t ctx;
    ctx.fd = fd;
    ctx.client_addr_len = sizeof(ctx.client_addr);
    memset(&ctx.client_addr, 0, sizeof(ctx.client_addr));

    // espera primeiro pacote
    unsigned char peek_buf[1];
    recvfrom(
        fd,
        peek_buf, sizeof(peek_buf),
        MSG_PEEK,
        (struct sockaddr *) &ctx.client_addr,
        &ctx.client_addr_len
    );

    printf("Client detected: %s:%d\n",
        inet_ntoa(ctx.client_addr.sin_addr),
        ntohs(ctx.client_addr.sin_port)
    );

    // identifica o cliente pro cookie
    mbedtls_ssl_set_client_transport_id(
        &ssl,
        (const unsigned char *) &ctx.client_addr,
        ctx.client_addr_len
    );

    mbedtls_ssl_set_bio(&ssl, &ctx, my_send, my_recv, NULL);

    printf("START HANDSHAKE\n");

    do {
        ret = mbedtls_ssl_handshake(&ssl);
    } while(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    // DTLS faz 2 rounds de handshake por causa do HelloVerifyRequest
    if(ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED)
    {
        printf("hello verification requested, resetting...\n");
        goto reset;
    }

    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("handshake failed: %d (%s)\n", ret, errbuf);
        close(fd);
        return 1;
    }

    printf("Ciphersuite: %s\n", mbedtls_ssl_get_ciphersuite(&ssl));

    printf("READ\n");

    unsigned char buf[1024];
    ret = mbedtls_ssl_read(&ssl, buf, sizeof(buf) - 1);
    if(ret > 0)
    {
        buf[ret] = '\0';
        printf("received %d bytes: %s\n", ret, (char *) buf);
    }

    mbedtls_ssl_close_notify(&ssl);

    close(fd);
    mbedtls_x509_crt_free(&srvcert);
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ssl_cookie_free(&cookie_ctx);

    return 0;
}