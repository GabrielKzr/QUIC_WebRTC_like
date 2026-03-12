#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mbedtls/ssl.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "psa/crypto.h"

static int my_send(void *ctx, const unsigned char *buf, size_t len)
{
    int fd = *(int *) ctx;
    ssize_t ret = send(fd, buf, len, 0);

    if(ret >= 0)
        return (int) ret;

    if(errno == EAGAIN || errno == EWOULDBLOCK)
        return MBEDTLS_ERR_SSL_WANT_WRITE;

    return -1;
}

static int my_recv(void *ctx, unsigned char *buf, size_t len)
{
    int fd = *(int *) ctx;
    ssize_t ret = recv(fd, buf, len, 0);

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

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_pk_init(&pkey);
    mbedtls_x509_crt_init(&srvcert);

    int ret = mbedtls_x509_crt_parse_file(&srvcert, "../cert.pem");
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("crt_parse failed: %d (%s)\n", ret, errbuf);
        return 1;
    }

    ret = mbedtls_pk_parse_keyfile(&pkey, "../key.pem", NULL);
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("key_parse failed: %d (%s)\n", ret, errbuf);
        return 1;
    }

    ret = mbedtls_ssl_config_defaults(
        &conf,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("config_defaults failed: %d (%s)\n", ret, errbuf);
        return 1;
    }

    ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey);
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("conf_own_cert failed: %d (%s)\n", ret, errbuf);
        return 1;
    }

    ret = mbedtls_ssl_setup(&ssl, &conf);
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("ssl_setup failed: %d (%s)\n", ret, errbuf);
        return 1;
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4433);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listen_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if(listen(listen_fd, 1) < 0)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Server listening on port 4433...\n");

    int client_fd = accept(listen_fd, NULL, NULL);
    if(client_fd < 0)
    {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    printf("Client connected!\n");

    mbedtls_ssl_set_bio(&ssl, &client_fd, my_send, my_recv, NULL);

    printf("START HANDSHAKE\n");

    ret = mbedtls_ssl_handshake(&ssl);
    if(ret != 0)
    {
        char errbuf[256];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        printf("handshake ret = %d (%s)\n", ret, errbuf);
        close(client_fd);
        close(listen_fd);
        return 1;
    }

    printf("READ\n");

    unsigned char buf[1024];
    ret = mbedtls_ssl_read(&ssl, buf, sizeof(buf) - 1);
    if(ret > 0)
    {
        buf[ret] = '\0';
        printf("received %d bytes: %s\n", ret, (char *) buf);
    }

    mbedtls_ssl_close_notify(&ssl);

    close(client_fd);
    close(listen_fd);
    mbedtls_x509_crt_free(&srvcert);
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);

    return 0;
}