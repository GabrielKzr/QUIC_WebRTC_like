#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include <stddef.h>
#include <netinet/in.h>
#include <sys/time.h>


typedef enum {
    UDP_TRANSPORT_OK = 0,
    UDP_TRANSPORT_ERROR = -1
} udp_transport_status_t;


enum {
    UDP_TRANSPORT_READABLE = 1,
    TCP_TRANSPORT_READABLE = 2
};


typedef struct udp_transport udp_transport_t;


/* Uma região por transporte, fornecida pelo chamador. Sem alocação dinâmica. */
typedef struct {
    int fd;
    int owns_fd;

    struct sockaddr_in peer;
} udp_transport_data_t;


/* A implementação controla a espera e consome seus controles internos.
 * UDP_TRANSPORT_READABLE significa datagrama disponível para o protocolo. */
typedef struct {
    udp_transport_status_t (*send)(
        udp_transport_t *transport, const void *buffer, size_t length
    );

    udp_transport_status_t (*recv)(
        udp_transport_t *transport, void *buffer, size_t capacity, size_t *received
    );

    udp_transport_status_t (*wait)(
        udp_transport_t *transport, const struct timeval *timeout, unsigned *events
    );

    udp_transport_status_t (*close)(udp_transport_t *transport);
} udp_transport_ops_t;


/* Inicialização: static udp_transport_data_t data;
 *                static udp_transport_t transport = {.data = &data};
 * Não copiar nem alterar uma instância ativa. ops == NULL indica inativa. */
struct udp_transport {
    const udp_transport_ops_t *ops;
    udp_transport_data_t *data;

    int tcp_fd;
};


typedef struct {
    int fd;                /* -1 cria UDP; >= 0 usa um FD UDP IPv4 fornecido. */
    int already_bound;
    int take_ownership;    /* FD fornecido: posse transferida só no sucesso. */
    int reuse_address;     /* SO_REUSEADDR antes do bind, se houver bind. */

    int tcp_fd;            /* -1 sem TCP. Apenas observado, nunca fechado aqui. */

    struct sockaddr_in local;
    struct sockaddr_in peer;
} udp_transport_udp_config_t;


/* Prepara UDP local; não comprova conectividade ou handshake do protocolo. */
udp_transport_status_t udp_transport_init_udp(
    udp_transport_t *transport,
    const udp_transport_udp_config_t *config
);


/* I/O não bloqueante por chamada. Qualquer falha retorna ERROR.
 * Datagramas vazios são válidos. recv sem dados ou truncado retorna ERROR
 * com received = 0. Truncamento consome o datagrama. Não filtra origem. */
udp_transport_status_t udp_transport_send(
    udp_transport_t *transport,
    const void *buffer,
    size_t length
);

udp_transport_status_t udp_transport_recv(
    udp_transport_t *transport,
    void *buffer,
    size_t capacity,
    size_t *received
);


/* Espera UDP e TCP opcional. timeout == NULL espera sem prazo.
 * Não modifica timeout. OK com events == 0 indica timeout; ambas as flags
 * podem vir juntas. O transporte processa eventos internos antes de sinalizar
 * dados ao protocolo. Não retorna notificações de controle ao chamador. */
udp_transport_status_t udp_transport_wait(
    udp_transport_t *transport,
    const struct timeval *timeout,
    unsigned *events
);


/* Atualizar ao aceitar/conectar TCP; usar -1 antes de fechar/desassociar TCP. */
udp_transport_status_t udp_transport_set_tcp_fd(
    udp_transport_t *transport,
    int tcp_fd
);


/* Idempotente. Fecha apenas UDP próprio e preserva a memória do chamador. */
udp_transport_status_t udp_transport_close(udp_transport_t *transport);

#endif
