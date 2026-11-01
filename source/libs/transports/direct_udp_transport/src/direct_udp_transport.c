#include "direct_udp_transport.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static udp_transport_status_t direct_wait(udp_transport_t *transport, unsigned requested_events, const struct timeval *timeout, unsigned *events);

static udp_transport_status_t direct_send(udp_transport_t *transport, const void *buffer, size_t length, const struct timeval *timeout) {
    direct_udp_transport_data_t *udp = transport->data;

    unsigned events = 0;
    udp_transport_status_t status = direct_wait(transport, UDP_TRANSPORT_WRITABLE, timeout, &events);
    if (status != UDP_TRANSPORT_OK) {
        return status;
    }
    if (!(events & UDP_TRANSPORT_WRITABLE)) {
        return UDP_TRANSPORT_NOT_READY;
    }

    ssize_t sent = sendto(
        udp->udp_fd, buffer, length, MSG_DONTWAIT,
        (const struct sockaddr *)&udp->peer, sizeof(udp->peer)
    );

    if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return UDP_TRANSPORT_NOT_READY;
    }

    if (sent < 0 || (size_t)sent != length) {
        return UDP_TRANSPORT_ERROR;
    }

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t direct_recv(udp_transport_t *transport, void *buffer, size_t capacity, size_t *received, const struct timeval *timeout) {
    direct_udp_transport_data_t *udp = transport->data;

    unsigned events = 0;
    udp_transport_status_t status = direct_wait(transport, UDP_TRANSPORT_READABLE, timeout, &events);
    if (status != UDP_TRANSPORT_OK) {
        return status;
    }
    if (!(events & UDP_TRANSPORT_READABLE)) {
        return UDP_TRANSPORT_NOT_READY;
    }
    struct sockaddr_in source = {0};

    struct iovec iov = {
        .iov_base = buffer,
        .iov_len = capacity
    };

    struct msghdr message = {
        .msg_name = &source,
        .msg_namelen = sizeof(source),
        .msg_iov = &iov,
        .msg_iovlen = 1
    };

    ssize_t count = recvmsg(udp->udp_fd, &message, MSG_DONTWAIT);

    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return UDP_TRANSPORT_NOT_READY;
    }

    if (count < 0 || (message.msg_flags & MSG_TRUNC)) {
        return UDP_TRANSPORT_ERROR;
    }

    if (message.msg_namelen != sizeof(source) ||
        source.sin_family != AF_INET ||
        source.sin_addr.s_addr != udp->peer.sin_addr.s_addr ||
        source.sin_port != udp->peer.sin_port) {
        return UDP_TRANSPORT_ERROR;
    }

    *received = (size_t)count;

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t direct_wait(udp_transport_t *transport, unsigned requested_events, const struct timeval *timeout, unsigned *events) {
    direct_udp_transport_data_t *udp = transport->data;
    if (udp == NULL || udp->udp_fd < 0 || udp->udp_fd >= FD_SETSIZE) {
        return UDP_TRANSPORT_ERROR;
    }

    int udp_fd = udp->udp_fd;
    int tcp_fd = (requested_events & (TCP_TRANSPORT_READABLE | TCP_TRANSPORT_WRITABLE))
                 ? transport->tcp_fd : -1;
    if (tcp_fd >= FD_SETSIZE) {
        return UDP_TRANSPORT_ERROR;
    }

    int max_fd = -1;
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    if (requested_events & UDP_TRANSPORT_READABLE) {
        FD_SET(udp_fd, &read_fds);
        max_fd = udp_fd;
    }
    if (requested_events & UDP_TRANSPORT_WRITABLE) {
        FD_SET(udp_fd, &write_fds);
        max_fd = udp_fd;
    }
    if ((requested_events & TCP_TRANSPORT_READABLE) && tcp_fd >= 0) {
        FD_SET(tcp_fd, &read_fds);
        if (tcp_fd > max_fd) max_fd = tcp_fd;
    }
    if ((requested_events & TCP_TRANSPORT_WRITABLE) && tcp_fd >= 0) {
        FD_SET(tcp_fd, &write_fds);
        if (tcp_fd > max_fd) max_fd = tcp_fd;
    }

    struct timeval wait_timeout;
    if (timeout != NULL) {
        wait_timeout = *timeout;
    }

    int ready = select(max_fd + 1, &read_fds, &write_fds, NULL,
                       timeout != NULL ? &wait_timeout : NULL);
    if (ready < 0) {
        return UDP_TRANSPORT_ERROR;
    }

    if (FD_ISSET(udp_fd, &read_fds)) {
        *events |= UDP_TRANSPORT_READABLE;
    }
    if (FD_ISSET(udp_fd, &write_fds)) {
        *events |= UDP_TRANSPORT_WRITABLE;
    }
    if (tcp_fd >= 0 && FD_ISSET(tcp_fd, &read_fds)) {
        *events |= TCP_TRANSPORT_READABLE;
    }
    if (tcp_fd >= 0 && FD_ISSET(tcp_fd, &write_fds)) {
        *events |= TCP_TRANSPORT_WRITABLE;
    }

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t direct_close(udp_transport_t *transport) {
    direct_udp_transport_data_t *udp = transport->data;
    int result = close(udp->udp_fd);

    *udp = (direct_udp_transport_data_t){.udp_fd = -1};

    return result == 0 ? UDP_TRANSPORT_OK : UDP_TRANSPORT_ERROR;
}

static udp_transport_status_t direct_open(udp_transport_t *transport) {
    if (transport == NULL || transport->tcp_fd >= FD_SETSIZE) {
        return UDP_TRANSPORT_ERROR;
    }

    const direct_udp_transport_config_t *config = transport->config;
    if (transport->data == NULL || config == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (config->local.sin_family != AF_INET ||
        config->peer.sin_family != AF_INET || config->peer.sin_port == 0) {
        return UDP_TRANSPORT_ERROR;
    }

    /* Só gravar na memória externa depois de concluir a inicialização. */
    direct_udp_transport_data_t udp = {.udp_fd = -1};
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (udp_fd < 0 || udp_fd >= FD_SETSIZE) {
        goto fail;
    }

    if (config->reuse_address) {
        int reuse = 1;

        if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            goto fail;
        }
    }

    if (bind(udp_fd, (const struct sockaddr *)&config->local, sizeof(config->local)) < 0) {
        goto fail;
    }

    udp.peer = config->peer;

    udp.udp_fd = udp_fd;
    *(direct_udp_transport_data_t *)transport->data = udp;

    return UDP_TRANSPORT_OK;

fail:
    if (udp_fd >= 0) {
        close(udp_fd);
    }

    return UDP_TRANSPORT_ERROR;
}

const udp_transport_ops_t direct_ops = {
    .open = direct_open,
    .send = direct_send,
    .recv = direct_recv,
    .wait = direct_wait,
    .close = direct_close
};
