#ifndef OSSL_QUIC_H
#define OSSL_QUIC_H

#include <openssl/ssl.h>
#include <openssl/quic.h>
#include <openssl/err.h>
#include "udp_conn.h"

#define size TCP_MTU_SIZE

typedef enum {
    OSSL_QUIC_UDP_CONNECTED, // triggered after connected
    OSSL_QUIC_UDP_DISCONNECTED,
    OSSL_QUIC_UDP_RECV_DATA, // triggered after receiveing data on udp
    OSSL_QUIC_TCP_DATA_SENT, // triggered after sending service data through udp conn
    OSSL_QUIC_UDP_DATA_SENT, // triggered after sending data through udp conn
} ossl_quic_reasons_t;

typedef enum {
    OSSL_TLS13_SUITE_AES_128_GCM_SHA256 = 0,
    OSSL_TLS13_SUITE_AES_256_GCM_SHA384,
    OSSL_TLS13_SUITE_CHACHA20_POLY1305_SHA256,
    OSSL_TLS13_SUITE_AES_128_CCM_SHA256,
    OSSL_TLS13_SUITE_AES_128_CCM_8_SHA256,
    OSSL_TLS13_MAX_SUITES = 5,
} ossl_tls13_suite_id_t;

typedef enum {
    OSSL_TLS13_PRESET_DEFAULT = 0,   /* OpenSSL default */
    OSSL_TLS13_PRESET_BALANCED,      /* AES256:CHACHA20:AES128 */
    OSSL_TLS13_PRESET_PERF_AES128,   /* AES128 primeiro */
    OSSL_TLS13_PRESET_CUSTOM
} ossl_tls13_preset_t;

typedef struct {
    int closed;
    SSL_CTX* ctx;
    SSL* listener;
    SSL* conn;
} ossl_quic_data_t;

/*
    @param tls13_preset permite escolher um preset disponível, ou criar um custom, se criar custom, definir ponteiro abaixo
    @param tls13_suites suites com prioridade definidas por ordem, 
        ex: tls13_suites[0] maior prioridade
            tls13_suites[tls13_suites_len-1] menor prioridade
    @param tls13_suites_len número de suites no vetor tls13_suites
    @param reuse Define se vai ou não reutilizar a sock naquele endereço
*/
typedef struct {
    ossl_tls13_preset_t tls13_preset;
    ossl_tls13_suite_id_t* tls13_suites;
    size_t tls13_suites_len;
    int reuse;
} ossl_quic_config_t;

extern udp_conn_generic_api_t ossl_quic_api;


#endif