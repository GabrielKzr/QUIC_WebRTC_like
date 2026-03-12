#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
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

int my_send(void *ctx, const unsigned char *buf, size_t len)
{
    printf("TLS record (%zu bytes):\n", len);

    for(size_t i = 0; i < len; i++)
        printf("%02x ", buf[i]);

    printf("\n\n");

    return len;
}

int my_recv(void *ctx, unsigned char *buf, size_t len)
{
    return -1;
}

int main()
{
    psa_crypto_init();

    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_net_init(&server_fd);

    mbedtls_ssl_config_defaults(
        &conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);

    mbedtls_ssl_setup(&ssl, &conf);

    int ret = mbedtls_net_connect(
        &server_fd,
        "127.0.0.1",
        "4433",
        MBEDTLS_NET_PROTO_TCP
    );

    if(ret != 0)
    {
        printf("connect failed %d\n", ret);
        return 1;
    }

    mbedtls_ssl_set_bio(
        &ssl,
        &server_fd,
        mbedtls_net_send,
        mbedtls_net_recv,
        NULL
    );

    printf("START HANDSHAKE\n");

    ret = mbedtls_ssl_handshake(&ssl);

    printf("handshake ret = %d\n", ret);

    // se quiser capturar pacote no meio do caminho
    // mbedtls_ssl_set_bio(&ssl, NULL, my_send, my_recv, NULL);

    printf("WRITE\n");

    char *msg = "Ola Mano\n";

    ret = mbedtls_ssl_write(&ssl, (unsigned char*)msg, strlen(msg));

    printf("write ret = %d\n", ret);

    mbedtls_ssl_close_notify(&ssl);

    return 0;
}