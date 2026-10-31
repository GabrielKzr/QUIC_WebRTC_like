#ifndef OSSL_QUIC_BIO_H
#define OSSL_QUIC_BIO_H

#include <netinet/in.h>
#include <openssl/bio.h>
#include "udp_transport.h"

/* O transporte deve entregar somente datagramas deste peer, como faz direct.
 * max_datagram_size limita o payload UDP, sem os cabeçalhos IP/UDP. */
typedef struct {
    udp_transport_t *transport;
    struct sockaddr_in peer;
    unsigned int max_datagram_size;
} ossl_quic_bio_config_t;

/* Liberar o método com BIO_meth_free somente após liberar seus BIOs. */
BIO_METHOD *ossl_quic_bio_method_new(void);

/* Configuração e transporte são emprestados e devem sobreviver ao BIO.
 * I/O por BIO_sendmmsg/BIO_recvmmsg, até um datagrama por chamada,
 * sem espera bloqueante ou posse do socket. */
BIO *ossl_quic_bio_new(const BIO_METHOD *method, const ossl_quic_bio_config_t *config);

#endif
