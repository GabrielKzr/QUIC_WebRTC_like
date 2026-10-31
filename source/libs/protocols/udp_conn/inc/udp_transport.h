#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include <stddef.h>
#include <sys/time.h>

typedef struct udp_transport udp_transport_t;

typedef enum {
    UDP_TRANSPORT_OK = 0,
    UDP_TRANSPORT_NOT_READY = 1,
    UDP_TRANSPORT_ERROR = -1
} udp_transport_status_t;


enum {
    UDP_TRANSPORT_READABLE = 1,
    TCP_TRANSPORT_READABLE = 2,
    UDP_TRANSPORT_WRITABLE = 4,
    TCP_TRANSPORT_WRITABLE = 8
};


typedef struct {
    udp_transport_status_t (*open)(udp_transport_t *transport);

    udp_transport_status_t (*send)(
        udp_transport_t *transport, const void *buffer, size_t length, const struct timeval *timeout
    );

    udp_transport_status_t (*recv)(
        udp_transport_t *transport, void *buffer, size_t capacity, size_t *received, const struct timeval *timeout
    );

    /* Consulta o prazo interno relativo, sem processar eventos.
     * has_timeout == 0 indica ausência de timer; == 1 torna timeout válido.
     * Prazo zero solicita espera sem bloqueio. Operação ausente significa sem timer.
     * O deadline fica nos dados da implementação: esta consulta calcula apenas
     * o restante; wait atende o vencimento e agenda o próximo prazo. */
    udp_transport_status_t (*get_timeout)(
        udp_transport_t *transport, struct timeval *timeout, int *has_timeout
    );

    udp_transport_status_t (*wait)(
        udp_transport_t *transport, unsigned requested_events, const struct timeval *timeout, unsigned *events
    );

    udp_transport_status_t (*close)(udp_transport_t *transport);
} udp_transport_ops_t;

struct udp_transport {
    const udp_transport_ops_t *ops;
    int opened;
    int tcp_fd; /* Emprestado; atualizar após accept/connect; -1 se ausente. */
    const void *config;
    void *data;
};

/* Despacha abertura para ops->open usando as configurações do handler.
 * A tabela ops permanece associada após close, permitindo reabertura. */
udp_transport_status_t udp_transport_open(udp_transport_t *transport);

/* Delega envio e espera à implementação: timeout zero consulta,
 * positivo limita a espera e NULL não impõe limite. Não modifica timeout nem
 * consulta/altera TCP. Timeout ou indisponibilidade temporária retorna NOT_READY;
 * falha de envio retorna ERROR.
 * Não consulta get_timeout. Não confirma entrega ao peer. */
udp_transport_status_t udp_transport_send(udp_transport_t *transport, const void *buffer, size_t length, const struct timeval *timeout);

/* Espera somente UDP antes de ler: timeout zero consulta sem bloquear,
 * positivo limita a espera e NULL não impõe limite. Não modifica timeout.
 * A implementação controla a espera, sem consultar get_timeout.
 * Timeout ou ausência de dados retorna NOT_READY; falha ou truncamento, ERROR.
 * Nos dois casos, received = 0.
 * Datagramas vazios são válidos. Truncamento consome o datagrama.
 * Não consulta nem altera tcp_fd e não fecha TCP. O transporte direto rejeita
 * origem diferente do peer configurado com ERROR/received = 0. */
udp_transport_status_t udp_transport_recv(udp_transport_t *transport, void *buffer, size_t capacity, size_t *received, const struct timeval *timeout);

/* Uma única espera pelos requested_events UDP/TCP de leitura/escrita:
 * consulta get_timeout, usa o menor prazo entre
 * transporte e chamador e delega a espera e o processamento a ops->wait.
 * NULL timeout não impõe prazo do chamador; get_timeout ainda pode impor um.
 * Não modifica timeout. Retorna somente eventos solicitados, que podem vir juntos.
 * TCP com fd == -1 não é observado. OK/events == 0 pode
 * indicar timeout ou controle consumido sem payload: o chamador decide repetir.
 * O loop e os prazos de protocolo pertencem a udp_conn. EINTR retorna ERROR. */
udp_transport_status_t udp_transport_wait(udp_transport_t *transport, unsigned requested_events, const struct timeval *timeout, unsigned *events);

/* Idempotente. Fecha apenas UDP próprio e preserva a memória do chamador. */
udp_transport_status_t udp_transport_close(udp_transport_t *transport);

#endif
