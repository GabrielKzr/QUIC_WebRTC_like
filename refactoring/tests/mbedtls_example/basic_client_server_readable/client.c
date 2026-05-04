#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include "psa/crypto.h"

/*
    Para rodar esse exemplo localmente, executar:

    $ openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes
    $ openssl s_server -accept 4433 -cert cert.pem -key key.pem

    Em outra janela, compile e execute o código abaixo com:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ ./client
*/

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

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);

    int ret = mbedtls_ssl_config_defaults(
        &conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);

    ret = mbedtls_ssl_setup(&ssl, &conf);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4433);

    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(server_fd, (struct sockaddr *) &addr, sizeof(addr));
    
    mbedtls_ssl_set_bio(&ssl, &server_fd, my_send, my_recv, NULL);

    printf("START HANDSHAKE\n");

    do
    {
        ret = mbedtls_ssl_handshake(&ssl);
    }
    while(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    printf("WRITE\n");

    const char *msg = "Ola Mano\n";

    do
    {
        ret = mbedtls_ssl_write(&ssl, (const unsigned char *) msg, strlen(msg));
    }
    while(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    printf("write ret = %d\n", ret);

    mbedtls_ssl_close_notify(&ssl);

    close(server_fd);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ssl_free(&ssl);

    return 0;
}