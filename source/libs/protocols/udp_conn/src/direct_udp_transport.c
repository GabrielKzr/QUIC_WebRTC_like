#include "direct_udp_transport.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static udp_transport_status_t direct_send(udp_transport_t *transport, const void *buffer, size_t length) {
    direct_udp_transport_data_t *udp = transport->data;

    ssize_t sent = sendto(
        transport->udp_fd, buffer, length, MSG_DONTWAIT,
        (const struct sockaddr *)&udp->peer, sizeof(udp->peer)
    );

    if (sent < 0 || (size_t)sent != length) {
        return UDP_TRANSPORT_ERROR;
    }

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t direct_recv(udp_transport_t *transport, void *buffer, size_t capacity, size_t *received) {
    struct iovec iov = {
        .iov_base = buffer,
        .iov_len = capacity
    };

    struct msghdr message = {
        .msg_iov = &iov,
        .msg_iovlen = 1
    };

    ssize_t count = recvmsg(transport->udp_fd, &message, MSG_DONTWAIT);

    if (count < 0 || (message.msg_flags & MSG_TRUNC)) {
        return UDP_TRANSPORT_ERROR;
    }

    *received = (size_t)count;

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t direct_get_timeout(udp_transport_t *transport, struct timeval *timeout, int *has_timeout) {
    (void)transport;
    *timeout = (struct timeval){0};
    *has_timeout = 0;

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t direct_dispatch(udp_transport_t *transport, udp_transport_wait_t *wait) {
    if (FD_ISSET(transport->udp_fd, &wait->read_fds)) {
        wait->events |= UDP_TRANSPORT_READABLE;
    }

    if (transport->tcp_fd >= 0 && FD_ISSET(transport->tcp_fd, &wait->read_fds)) {
        wait->events |= TCP_TRANSPORT_READABLE;
    }

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t direct_close(udp_transport_t *transport) {
    direct_udp_transport_data_t *udp = transport->data;
    int result = 0;

    if (udp->owns_fd) {
        result = close(transport->udp_fd);
    }

    *udp = (direct_udp_transport_data_t){0};
    transport->udp_fd = -1;

    return result == 0 ? UDP_TRANSPORT_OK : UDP_TRANSPORT_ERROR;
}

static udp_transport_status_t direct_open(udp_transport_t *transport) {
    if (transport == NULL || transport->transport_config == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    const direct_udp_transport_config_t *config = transport->config;
    const udp_transport_config_t *common = transport->transport_config;
    if (transport->data == NULL || config == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (common->udp_fd < -1 || common->udp_fd >= FD_SETSIZE ||
        config->local.sin_family != AF_INET ||
        config->peer.sin_family != AF_INET || config->peer.sin_port == 0 ||
        (common->udp_fd == -1 && config->already_bound)) {
        return UDP_TRANSPORT_ERROR;
    }

    /* Só gravar na memória externa depois de concluir a inicialização. */
    direct_udp_transport_data_t udp = {0};
    int udp_fd = common->udp_fd;

    if (udp_fd == -1) {
        udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (udp_fd < 0 || udp_fd >= FD_SETSIZE) {
        goto fail;
    }

    int type;
    socklen_t type_length = sizeof(type);

    struct sockaddr_in local;
    socklen_t local_length = sizeof(local);

    if (getsockopt(udp_fd, SOL_SOCKET, SO_TYPE, &type, &type_length) < 0 ||
        getsockname(udp_fd, (struct sockaddr *)&local, &local_length) < 0) {
        goto fail;
    }

    if (type != SOCK_DGRAM || local.sin_family != AF_INET ||
        (config->already_bound && local.sin_port == 0)) {
        goto fail;
    }

    if (!config->already_bound) {
        if (config->reuse_address) {
            int reuse = 1;

            if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
                goto fail;
            }
        }

        if (bind(udp_fd, (const struct sockaddr *)&config->local, sizeof(config->local)) < 0) {
            goto fail;
        }
    }

    udp.owns_fd = common->udp_fd == -1 || config->take_ownership;
    udp.peer = config->peer;

    *(direct_udp_transport_data_t *)transport->data = udp;
    transport->udp_fd = udp_fd;

    return UDP_TRANSPORT_OK;

fail:
    if (common->udp_fd == -1 && udp_fd >= 0) {
        close(udp_fd);
    }

    return UDP_TRANSPORT_ERROR;
}

const udp_transport_ops_t direct_ops = {
    .open = direct_open,
    .send = direct_send,
    .recv = direct_recv,
    .get_timeout = direct_get_timeout,
    .dispatch = direct_dispatch,
    .close = direct_close
};
