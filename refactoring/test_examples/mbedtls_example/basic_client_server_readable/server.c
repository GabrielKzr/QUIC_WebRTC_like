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
    printf("entrei no send\n");

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
    printf("entrei no recv\n");

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

    int ret = mbedtls_x509_crt_parse_file(&srvcert, "../../../../common/cert.pem");
    ret = mbedtls_pk_parse_keyfile(&pkey, "../../../../common/key.pem", NULL);
    ret = mbedtls_ssl_config_defaults(
        &conf,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );

    ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey);
    ret = mbedtls_ssl_setup(&ssl, &conf);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4433);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listen_fd, (struct sockaddr *) &addr, sizeof(addr));
    listen(listen_fd, 1);

    printf("Server listening on port 4433...\n");

    int client_fd = accept(listen_fd, NULL, NULL);

    printf("Client connected!\n");

    mbedtls_ssl_set_bio(&ssl, &client_fd, my_send, my_recv, NULL);

    printf("START HANDSHAKE\n");

    ret = mbedtls_ssl_handshake(&ssl);

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