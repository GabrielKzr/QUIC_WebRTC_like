#include "ossl_quic_bio.h"

#include <openssl/err.h>

static int ossl_bio_create(BIO *bio) {
    BIO_set_data(bio, NULL);
    BIO_set_init(bio, 0);
    BIO_set_shutdown(bio, 0);
    return 1;
}

static int ossl_bio_destroy(BIO *bio) {
    if(bio == NULL) return 0;

    BIO_set_data(bio, NULL);
    BIO_set_init(bio, 0);
    return 1;
}

static long ossl_bio_ctrl(BIO *bio, int command, long value, void *pointer) {
    const ossl_quic_bio_config_t *config = BIO_get_data(bio);

    switch(command) {
    case BIO_CTRL_FLUSH:
        return 1;

    case BIO_CTRL_DGRAM_GET_PEER:
        if(config == NULL || pointer == NULL) return 0;
        if(!BIO_ADDR_rawmake(pointer, config->peer.sin_family,
                            &config->peer.sin_addr, sizeof(config->peer.sin_addr),
                            config->peer.sin_port))
            return 0;
        return sizeof(config->peer);

    case BIO_CTRL_DGRAM_GET_MTU:
    case BIO_CTRL_DGRAM_QUERY_MTU:
    case BIO_CTRL_DGRAM_GET_FALLBACK_MTU:
        return config != NULL ? config->max_datagram_size : 0;

    case BIO_CTRL_DGRAM_GET_MTU_OVERHEAD:
        /* O limite configurado já representa somente o payload UDP. */
        return 0;

    case BIO_CTRL_DGRAM_GET_CAPS:
    case BIO_CTRL_DGRAM_GET_EFFECTIVE_CAPS:
        return BIO_DGRAM_CAP_HANDLES_DST_ADDR |
               BIO_DGRAM_CAP_PROVIDES_SRC_ADDR;

    case BIO_CTRL_DGRAM_GET_LOCAL_ADDR_CAP:
        return 0;

    case BIO_CTRL_DGRAM_GET_LOCAL_ADDR_ENABLE:
        if(pointer == NULL) return 0;
        *(int *)pointer = 0;
        return 1;

    case BIO_CTRL_DGRAM_SET_LOCAL_ADDR_ENABLE:
        return value == 0;

    case BIO_CTRL_DGRAM_GET_NO_TRUNC:
        return 1;

    case BIO_CTRL_DGRAM_SET_NO_TRUNC:
        return value == 1;

    case BIO_C_GET_FD:
        if(pointer != NULL) *(int *)pointer = -1;
        return -1;

    case BIO_CTRL_GET_RPOLL_DESCRIPTOR:
    case BIO_CTRL_GET_WPOLL_DESCRIPTOR:
        if(pointer != NULL)
            *(BIO_POLL_DESCRIPTOR *)pointer = (BIO_POLL_DESCRIPTOR){0};
        return 0;

    default:
        return 0;
    }
}

static int ossl_bio_sendmmsg(BIO *bio, BIO_MSG *messages, size_t stride,
                           size_t count, uint64_t flags, size_t *processed) {
    const ossl_quic_bio_config_t *config = BIO_get_data(bio);
    const struct timeval timeout = {0};

    if(processed == NULL) {
        ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
        return 0;
    }
    *processed = 0;

    if(flags != 0 || stride != sizeof(*messages)) {
        ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
        return 0;
    }
    if(count == 0) return 1;

    if(config == NULL) {
        ERR_raise(ERR_LIB_BIO, BIO_R_UNINITIALIZED);
        return 0;
    }
    if(messages == NULL || messages[0].flags != 0 ||
       (messages[0].data == NULL && messages[0].data_len != 0)) {
        ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
        return 0;
    }
    if(messages[0].local != NULL) {
        ERR_raise(ERR_LIB_BIO, BIO_R_LOCAL_ADDR_NOT_AVAILABLE);
        return 0;
    }
    if(messages[0].data_len > config->max_datagram_size) {
        ERR_raise(ERR_LIB_BIO, BIO_R_LENGTH_TOO_LONG);
        return 0;
    }

    if(messages[0].peer != NULL) {
        struct in_addr address;
        size_t length = sizeof(address);

        if(BIO_ADDR_family(messages[0].peer) != AF_INET ||
           !BIO_ADDR_rawaddress(messages[0].peer, &address, &length) ||
           length != sizeof(address) ||
           address.s_addr != config->peer.sin_addr.s_addr ||
           BIO_ADDR_rawport(messages[0].peer) != config->peer.sin_port) {
            ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
            return 0;
        }
    }

    udp_transport_status_t status = udp_transport_send(
        config->transport, messages[0].data, messages[0].data_len, &timeout
    );

    if(status != UDP_TRANSPORT_OK) {
        ERR_raise(ERR_LIB_BIO, status == UDP_TRANSPORT_NOT_READY
                  ? BIO_R_NON_FATAL : BIO_R_TRANSFER_ERROR);
        return 0;
    }

    messages[0].flags = 0;
    *processed = 1;
    return 1;
}

static int ossl_bio_recvmmsg(BIO *bio, BIO_MSG *messages, size_t stride,
                           size_t count, uint64_t flags, size_t *processed) {
    const ossl_quic_bio_config_t *config = BIO_get_data(bio);
    const struct timeval timeout = {0};
    size_t received = 0;

    if(processed == NULL) {
        ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
        return 0;
    }
    *processed = 0;

    if(flags != 0 || stride != sizeof(*messages)) {
        ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
        return 0;
    }
    if(count == 0) return 1;

    if(config == NULL) {
        ERR_raise(ERR_LIB_BIO, BIO_R_UNINITIALIZED);
        return 0;
    }
    if(messages == NULL || messages[0].flags != 0 ||
       (messages[0].data == NULL && messages[0].data_len != 0)) {
        ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
        return 0;
    }
    if(messages[0].local != NULL) {
        ERR_raise(ERR_LIB_BIO, BIO_R_LOCAL_ADDR_NOT_AVAILABLE);
        return 0;
    }

    udp_transport_status_t status = udp_transport_recv(
        config->transport, messages[0].data, messages[0].data_len,
        &received, &timeout
    );

    if(status != UDP_TRANSPORT_OK) {
        ERR_raise(ERR_LIB_BIO, status == UDP_TRANSPORT_NOT_READY
                  ? BIO_R_NON_FATAL : BIO_R_TRANSFER_ERROR);
        return 0;
    }

    if(messages[0].peer != NULL &&
       !BIO_ADDR_rawmake(messages[0].peer, config->peer.sin_family,
                         &config->peer.sin_addr, sizeof(config->peer.sin_addr),
                         config->peer.sin_port)) {
        ERR_raise(ERR_LIB_BIO, BIO_R_TRANSFER_ERROR);
        return 0;
    }

    messages[0].data_len = received;
    messages[0].flags = 0;
    *processed = 1;
    return 1;
}

BIO_METHOD *ossl_quic_bio_method_new(void) {
    int index = BIO_get_new_index();
    if(index < 0) {
        ERR_raise(ERR_LIB_BIO, ERR_R_INTERNAL_ERROR);
        return NULL;
    }

    BIO_METHOD *method = BIO_meth_new(index | BIO_TYPE_SOURCE_SINK, "udp_transport");
    if(method == NULL) return NULL;

    if(!BIO_meth_set_create(method, ossl_bio_create) ||
       !BIO_meth_set_destroy(method, ossl_bio_destroy) ||
       !BIO_meth_set_ctrl(method, ossl_bio_ctrl) ||
       !BIO_meth_set_sendmmsg(method, ossl_bio_sendmmsg) ||
       !BIO_meth_set_recvmmsg(method, ossl_bio_recvmmsg)) {
        BIO_meth_free(method);
        return NULL;
    }

    return method;
}

BIO *ossl_quic_bio_new(const BIO_METHOD *method, const ossl_quic_bio_config_t *config) {
    if(method == NULL || config == NULL || config->transport == NULL ||
       !config->transport->opened || config->transport->ops == NULL ||
       config->transport->ops->send == NULL || config->transport->ops->recv == NULL ||
       config->peer.sin_family != AF_INET || config->peer.sin_port == 0 ||
       config->max_datagram_size < 1200 || config->max_datagram_size > 65507) {
        ERR_raise(ERR_LIB_BIO, BIO_R_INVALID_ARGUMENT);
        return NULL;
    }

    BIO *bio = BIO_new(method);
    if(bio == NULL) return NULL;

    BIO_set_data(bio, (void *)config);
    BIO_set_init(bio, 1);
    return bio;
}
