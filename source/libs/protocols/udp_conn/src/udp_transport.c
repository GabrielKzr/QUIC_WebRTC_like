#include "udp_transport.h"

#include <sys/select.h>

static int is_active(const udp_transport_t *transport) {
    return transport != NULL && transport->ops != NULL && transport->opened;
}

udp_transport_status_t udp_transport_open(udp_transport_t *transport) {
    if (transport == NULL || transport->ops == NULL ||
        transport->ops->open == NULL || transport->opened ||
        transport->transport_config == NULL ||
        transport->transport_config->tcp_fd < -1 ||
        transport->transport_config->tcp_fd >= FD_SETSIZE) {
        return UDP_TRANSPORT_ERROR;
    }

    udp_transport_status_t status = transport->ops->open(transport);

    if (status == UDP_TRANSPORT_OK) {
        transport->tcp_fd = transport->transport_config->tcp_fd;
        transport->opened = 1;
    }

    return status;
}

udp_transport_status_t udp_transport_send(udp_transport_t *transport, const void *buffer, size_t length) {
    if (!is_active(transport) || (buffer == NULL && length != 0)) {
        return UDP_TRANSPORT_ERROR;
    }

    return transport->ops->send(transport, buffer, length);
}

udp_transport_status_t udp_transport_recv(udp_transport_t *transport, void *buffer, size_t capacity, size_t *received) {
    if (received != NULL) {
        *received = 0;
    }

    if (!is_active(transport) || received == NULL ||
        (buffer == NULL && capacity != 0)) {
        return UDP_TRANSPORT_ERROR;
    }

    return transport->ops->recv(transport, buffer, capacity, received);
}

static int valid_timeout(const struct timeval *timeout) {
    return timeout->tv_sec >= 0 && timeout->tv_usec >= 0 &&
           timeout->tv_usec < 1000000;
}

static udp_transport_status_t get_timeout(udp_transport_t *transport, struct timeval *timeout, int *has_timeout) {
    *timeout = (struct timeval){0};
    *has_timeout = 0;

    if (transport->ops->get_timeout == NULL) {
        return UDP_TRANSPORT_OK;
    }

    struct timeval internal = {0};
    int has_internal = 0;

    if (transport->ops->get_timeout(transport, &internal, &has_internal) != UDP_TRANSPORT_OK ||
        (has_internal && !valid_timeout(&internal))) {
        return UDP_TRANSPORT_ERROR;
    }

    if (has_internal) {
        *timeout = internal;
        *has_timeout = 1;
    }

    return UDP_TRANSPORT_OK;
}

udp_transport_status_t udp_transport_wait(udp_transport_t *transport, const struct timeval *timeout, unsigned *events) {
    if (events != NULL) {
        *events = 0;
    }

    if (!is_active(transport) || events == NULL ||
        transport->ops->dispatch == NULL ||
        transport->udp_fd < 0 || transport->udp_fd >= FD_SETSIZE ||
        transport->tcp_fd < -1 || transport->tcp_fd >= FD_SETSIZE) {
        return UDP_TRANSPORT_ERROR;
    }

    if (timeout != NULL && !valid_timeout(timeout)) {
        return UDP_TRANSPORT_ERROR;
    }

    udp_transport_wait_t wait = {0};
    struct timeval wait_timeout;
    int has_timeout;

    if (get_timeout(transport, &wait_timeout, &has_timeout) != UDP_TRANSPORT_OK) {
        return UDP_TRANSPORT_ERROR;
    }

    if (timeout != NULL && (!has_timeout || timeout->tv_sec < wait_timeout.tv_sec ||
        (timeout->tv_sec == wait_timeout.tv_sec && timeout->tv_usec < wait_timeout.tv_usec))) {
        wait_timeout = *timeout;
        has_timeout = 1;
    }

    int udp_fd = transport->udp_fd;
    int tcp_fd = transport->tcp_fd;
    int max_fd = udp_fd;

    FD_ZERO(&wait.read_fds);
    FD_SET(udp_fd, &wait.read_fds);

    if (tcp_fd >= 0) {
        FD_SET(tcp_fd, &wait.read_fds);

        if (tcp_fd > max_fd) {
            max_fd = tcp_fd;
        }
    }

    int ready = select(max_fd + 1, &wait.read_fds, NULL, NULL,
                       has_timeout ? &wait_timeout : NULL);

    if (ready < 0) {
        return UDP_TRANSPORT_ERROR;
    }

    if (transport->ops->dispatch(transport, &wait) != UDP_TRANSPORT_OK) {
        return UDP_TRANSPORT_ERROR;
    }

    *events = wait.events & (UDP_TRANSPORT_READABLE | TCP_TRANSPORT_READABLE);
    return UDP_TRANSPORT_OK;
}

udp_transport_status_t udp_transport_close(udp_transport_t *transport) {
    if (transport == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (!transport->opened) {
        return UDP_TRANSPORT_OK;
    }

    udp_transport_status_t status = transport->ops->close(transport);
    transport->opened = 0;
    transport->tcp_fd = -1;

    return status;
}
