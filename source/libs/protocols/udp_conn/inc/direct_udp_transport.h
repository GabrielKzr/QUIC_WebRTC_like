#ifndef DIRECT_UDP_TRANSPORT_H
#define DIRECT_UDP_TRANSPORT_H

#include "udp_transport.h"
#include <netinet/in.h>

/* Armazenamento fornecido pelo chamador, sem alocação dinâmica. */
typedef struct {
    int owns_fd;
    struct sockaddr_in peer;
} direct_udp_transport_data_t;

typedef struct {
    int already_bound;
    int take_ownership;    /* Posse transferida somente no sucesso. */
    int reuse_address;
    struct sockaddr_in local;
    struct sockaddr_in peer;
} direct_udp_transport_config_t;

/* Selecionar no handler: .ops = &direct_ops, com config e data próprios.
 * Abertura via udp_transport_open; fechamento preserva a memória do chamador. */
extern const udp_transport_ops_t direct_ops;

#endif
