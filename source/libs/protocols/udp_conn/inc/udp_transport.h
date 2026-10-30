#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include <stddef.h>
#include <sys/time.h>
#include <sys/select.h>

typedef struct udp_transport udp_transport_t;

typedef enum {
    UDP_TRANSPORT_OK = 0,
    UDP_TRANSPORT_ERROR = -1
} udp_transport_status_t;


enum {
    UDP_TRANSPORT_READABLE = 1,
    TCP_TRANSPORT_READABLE = 2
};


/* dispatch lê read_fds retornado pelo select e preenche os eventos UDP/TCP.
 * Controles UDP são consumidos sem sinalizar payload ao chamador.
 * Também atende timers vencidos, inclusive com read_fds vazio após timeout.
 * A montagem dos conjuntos e o select pertencem à camada comum. */
typedef struct {
    fd_set read_fds;
    unsigned events;
} udp_transport_wait_t;

typedef struct {
    udp_transport_status_t (*open)(udp_transport_t *transport);

    udp_transport_status_t (*send)(
        udp_transport_t *transport, const void *buffer, size_t length
    );

    udp_transport_status_t (*recv)(
        udp_transport_t *transport, void *buffer, size_t capacity, size_t *received
    );

    /* Consulta o prazo interno relativo, sem processar eventos.
     * has_timeout == 0 indica ausência de timer; == 1 torna timeout válido.
     * Prazo zero solicita select sem bloqueio. Operação ausente significa sem timer.
     * O deadline fica nos dados da implementação: esta consulta calcula apenas
     * o restante; dispatch atende o vencimento e agenda o próximo prazo. */
    udp_transport_status_t (*get_timeout)(
        udp_transport_t *transport, struct timeval *timeout, int *has_timeout
    );

    udp_transport_status_t (*dispatch)(
        udp_transport_t *transport, udp_transport_wait_t *wait
    );

    udp_transport_status_t (*close)(udp_transport_t *transport);
} udp_transport_ops_t;

/* Configuração compartilhada por todas as implementações. */
typedef struct {
    int udp_fd; /* -1 solicita criação pela implementação. */
    int tcp_fd; /* -1 sem TCP; emprestado, nunca fechado pelo transporte. */
} udp_transport_config_t;

struct udp_transport {
    const udp_transport_ops_t *ops;
    int opened;
    int udp_fd;
    int tcp_fd; /* Atualizar após accept/connect; -1 antes de fechar/desassociar. */
    const udp_transport_config_t *transport_config;
    const void *config;
    void *data;
};

/* Despacha abertura para ops->open usando as configurações do handler.
 * A tabela ops permanece associada após close, permitindo reabertura. */
udp_transport_status_t udp_transport_open(udp_transport_t *transport);

/* I/O não bloqueante por chamada. Qualquer falha retorna ERROR.
 * Datagramas vazios são válidos. recv sem dados ou truncado retorna ERROR
 * com received = 0. Truncamento consome o datagrama. Não filtra origem. */
udp_transport_status_t udp_transport_send(udp_transport_t *transport, const void *buffer, size_t length);

udp_transport_status_t udp_transport_recv(udp_transport_t *transport, void *buffer, size_t capacity, size_t *received);

/* Uma única espera UDP/TCP: consulta get_timeout, usa o menor prazo entre
 * transporte e chamador, executa select e chama dispatch uma vez após sucesso.
 * NULL timeout não impõe prazo do chamador; get_timeout ainda pode impor um.
 * Não modifica timeout. Ambas as flags podem vir juntas. OK/events == 0 pode
 * indicar timeout ou controle consumido sem payload: o chamador decide repetir.
 * O loop e os prazos de protocolo pertencem a udp_conn. EINTR retorna ERROR. */
udp_transport_status_t udp_transport_wait(udp_transport_t *transport, const struct timeval *timeout, unsigned *events);

/* Idempotente. Fecha apenas UDP próprio e preserva a memória do chamador. */
udp_transport_status_t udp_transport_close(udp_transport_t *transport);

#endif
