#include "udp_transport.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>


static const udp_transport_ops_t direct_ops;


static udp_transport_status_t direct_send(
    udp_transport_t *transport,
    const void *buffer,
    size_t length
) {
    udp_transport_data_t *udp = transport->data;

    ssize_t sent = sendto(
        udp->fd, buffer, length, MSG_DONTWAIT,
        (const struct sockaddr *)&udp->peer, sizeof(udp->peer)
    );

    if (sent < 0 || (size_t)sent != length) {
        return UDP_TRANSPORT_ERROR;
    }

    return UDP_TRANSPORT_OK;
}


static udp_transport_status_t direct_recv(
    udp_transport_t *transport,
    void *buffer,
    size_t capacity,
    size_t *received
) {
    struct iovec iov = {
        .iov_base = buffer,
        .iov_len = capacity
    };

    struct msghdr message = {
        .msg_iov = &iov,
        .msg_iovlen = 1
    };

    ssize_t count = recvmsg(transport->data->fd, &message, MSG_DONTWAIT);

    if (count < 0 || (message.msg_flags & MSG_TRUNC)) {
        return UDP_TRANSPORT_ERROR;
    }

    *received = (size_t)count;

    return UDP_TRANSPORT_OK;
}


static udp_transport_status_t direct_wait(
    udp_transport_t *transport,
    const struct timeval *timeout,
    unsigned *events
) {
    int udp_fd = transport->data->fd;
    int tcp_fd = transport->tcp_fd;

    if (udp_fd < 0 || udp_fd >= FD_SETSIZE ||
        tcp_fd < -1 || tcp_fd >= FD_SETSIZE) {
        return UDP_TRANSPORT_ERROR;
    }

    fd_set read_fds;

    FD_ZERO(&read_fds);
    FD_SET(udp_fd, &read_fds);

    int max_fd = udp_fd;

    if (tcp_fd >= 0) {
        FD_SET(tcp_fd, &read_fds);

        if (tcp_fd > max_fd) {
            max_fd = tcp_fd;
        }
    }

    struct timeval remaining;
    struct timeval *wait_timeout = NULL;

    if (timeout != NULL) {
        remaining = *timeout;
        wait_timeout = &remaining;
    }

    int ready = select(max_fd + 1, &read_fds, NULL, NULL, wait_timeout);

    if (ready < 0) {
        return UDP_TRANSPORT_ERROR;
    }

    if (FD_ISSET(udp_fd, &read_fds)) {
        *events |= UDP_TRANSPORT_READABLE;
    }

    if (tcp_fd >= 0 && FD_ISSET(tcp_fd, &read_fds)) {
        *events |= TCP_TRANSPORT_READABLE;
    }

    return UDP_TRANSPORT_OK;
}


static udp_transport_status_t direct_close(udp_transport_t *transport) {
    udp_transport_data_t *udp = transport->data;
    int result = 0;

    if (udp->owns_fd) {
        result = close(udp->fd);
    }

    *udp = (udp_transport_data_t){.fd = -1};

    transport->ops = NULL;
    transport->tcp_fd = -1;

    return result == 0 ? UDP_TRANSPORT_OK : UDP_TRANSPORT_ERROR;
}


udp_transport_status_t udp_transport_init_udp(
    udp_transport_t *transport,
    const udp_transport_udp_config_t *config
) {
    if (transport == NULL || transport->data == NULL || config == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (transport->ops != NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (config->fd < -1 || config->fd >= FD_SETSIZE ||
        config->tcp_fd < -1 || config->tcp_fd >= FD_SETSIZE ||
        config->local.sin_family != AF_INET ||
        config->peer.sin_family != AF_INET || config->peer.sin_port == 0 ||
        (config->fd == -1 && config->already_bound)) {
        return UDP_TRANSPORT_ERROR;
    }

    /* Só gravar na memória externa depois de concluir a inicialização. */
    udp_transport_data_t udp = {.fd = config->fd};

    if (udp.fd == -1) {
        udp.fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (udp.fd < 0 || udp.fd >= FD_SETSIZE) {
        goto fail;
    }

    int type;
    socklen_t type_length = sizeof(type);

    struct sockaddr_in local;
    socklen_t local_length = sizeof(local);

    if (getsockopt(udp.fd, SOL_SOCKET, SO_TYPE, &type, &type_length) < 0 ||
        getsockname(udp.fd, (struct sockaddr *)&local, &local_length) < 0) {
        goto fail;
    }

    if (type != SOCK_DGRAM || local.sin_family != AF_INET ||
        (config->already_bound && local.sin_port == 0)) {
        goto fail;
    }

    if (!config->already_bound) {
        if (config->reuse_address) {
            int reuse = 1;

            if (setsockopt(udp.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
                goto fail;
            }
        }

        if (bind(udp.fd, (const struct sockaddr *)&config->local, sizeof(config->local)) < 0) {
            goto fail;
        }
    }

    udp.owns_fd = config->fd == -1 || config->take_ownership;
    udp.peer = config->peer;

    *transport->data = udp;
    transport->tcp_fd = config->tcp_fd;
    transport->ops = &direct_ops;

    return UDP_TRANSPORT_OK;

fail:
    if (config->fd == -1 && udp.fd >= 0) {
        close(udp.fd);
    }

    return UDP_TRANSPORT_ERROR;
}


static int is_active(const udp_transport_t *transport) {
    return transport != NULL && transport->ops != NULL && transport->data != NULL;
}


udp_transport_status_t udp_transport_send(
    udp_transport_t *transport,
    const void *buffer,
    size_t length
) {
    if (!is_active(transport) || (buffer == NULL && length != 0)) {
        return UDP_TRANSPORT_ERROR;
    }

    return transport->ops->send(transport, buffer, length);
}


udp_transport_status_t udp_transport_recv(
    udp_transport_t *transport,
    void *buffer,
    size_t capacity,
    size_t *received
) {
    if (received != NULL) {
        *received = 0;
    }

    if (!is_active(transport) || received == NULL ||
        (buffer == NULL && capacity != 0)) {
        return UDP_TRANSPORT_ERROR;
    }

    return transport->ops->recv(transport, buffer, capacity, received);
}


udp_transport_status_t udp_transport_wait(
    udp_transport_t *transport,
    const struct timeval *timeout,
    unsigned *events
) {
    if (events != NULL) {
        *events = 0;
    }

    if (!is_active(transport) || events == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    return transport->ops->wait(transport, timeout, events);
}


udp_transport_status_t udp_transport_set_tcp_fd(
    udp_transport_t *transport,
    int tcp_fd
) {
    if (!is_active(transport) || tcp_fd < -1 || tcp_fd >= FD_SETSIZE) {
        return UDP_TRANSPORT_ERROR;
    }

    transport->tcp_fd = tcp_fd;

    return UDP_TRANSPORT_OK;
}


udp_transport_status_t udp_transport_close(udp_transport_t *transport) {
    if (transport == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (transport->ops == NULL) {
        return UDP_TRANSPORT_OK;
    }

    if (transport->data == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    return transport->ops->close(transport);
}


static const udp_transport_ops_t direct_ops = {
    .send = direct_send,
    .recv = direct_recv,
    .wait = direct_wait,
    .close = direct_close
};
