#include "udp_transport.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static int bound_socket(int type, struct sockaddr_in *address) {
    int fd = socket(AF_INET, type, 0);
    CHECK(fd >= 0 && fd < FD_SETSIZE);
    *address = (struct sockaddr_in){.sin_family = AF_INET,
                                  .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    CHECK(bind(fd, (struct sockaddr *)address, sizeof(*address)) == 0);
    socklen_t length = sizeof(*address);
    CHECK(getsockname(fd, (struct sockaddr *)address, &length) == 0);
    return fd;
}

static unsigned wait_transport(udp_transport_t *transport) {
    struct timeval timeout = {.tv_sec = 1};
    unsigned events;

    CHECK(udp_transport_wait(transport, &timeout, &events) == UDP_TRANSPORT_OK);
    CHECK(timeout.tv_sec == 1 && timeout.tv_usec == 0);

    return events;
}

int main(void) {
    struct sockaddr_in address_a, address_b;
    int fd_a = bound_socket(SOCK_DGRAM, &address_a);
    int fd_b = bound_socket(SOCK_DGRAM, &address_b);
    int original_flags = fcntl(fd_a, F_GETFL);
    CHECK(original_flags >= 0);
    static udp_transport_data_t data_a, data_b;
    udp_transport_t a = {.data = &data_a}, b = {.data = &data_b};
    udp_transport_udp_config_t config_a = {
        .fd = fd_a, .already_bound = 1, .tcp_fd = -1,
        .local = address_a, .peer = address_b
    };
    udp_transport_udp_config_t config_b = {
        .fd = fd_b, .already_bound = 1, .take_ownership = 1, .tcp_fd = -1,
        .local = address_b, .peer = address_a
    };
    udp_transport_t without_storage = {0};
    CHECK(udp_transport_init_udp(&without_storage, &config_a) == UDP_TRANSPORT_ERROR);
    CHECK(fcntl(fd_a, F_GETFD) >= 0);
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK); /* Dados static ainda zerados. */
    CHECK(a.data == &data_a && fcntl(fd_a, F_GETFD) >= 0);
    CHECK(udp_transport_init_udp(&a, &config_a) == UDP_TRANSPORT_OK);
    CHECK(udp_transport_init_udp(&b, &config_b) == UDP_TRANSPORT_OK);
    CHECK(a.tcp_fd == -1);
    CHECK(udp_transport_init_udp(&a, &config_a) == UDP_TRANSPORT_ERROR);

    char buffer[32];
    size_t received = 99;
    CHECK(udp_transport_recv(&a, buffer, sizeof(buffer), &received) == UDP_TRANSPORT_ERROR);
    CHECK(received == 0 && fcntl(fd_a, F_GETFL) == original_flags);

    /* Datagramas binários e um socket TCP na mesma espera, sem threads. */
    struct sockaddr_in tcp_address;
    int listener = bound_socket(SOCK_STREAM, &tcp_address);
    CHECK(listen(listener, 1) == 0);
    int tcp_client = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(tcp_client >= 0);
    CHECK(connect(tcp_client, (struct sockaddr *)&tcp_address, sizeof(tcp_address)) == 0);
    int tcp_server = accept(listener, NULL, NULL);
    CHECK(tcp_server >= 0 && tcp_server < FD_SETSIZE);

    const char payload[] = {'a', '\0', 'b', '\xff'};
    CHECK(udp_transport_send(&a, payload, sizeof(payload)) == UDP_TRANSPORT_OK);
    CHECK(send(tcp_client, "t", 1, 0) == 1);

    CHECK(udp_transport_set_tcp_fd(&b, tcp_server) == UDP_TRANSPORT_OK);

    unsigned events;

    events = wait_transport(&b);
    CHECK(events == (UDP_TRANSPORT_READABLE | TCP_TRANSPORT_READABLE));

    CHECK(recv(tcp_server, buffer, 1, 0) == 1 && buffer[0] == 't');
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received) == UDP_TRANSPORT_OK);
    CHECK(received == sizeof(payload) && memcmp(buffer, payload, received) == 0);
    CHECK(udp_transport_send(&b, "reply", 5) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&a) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&a, buffer, sizeof(buffer), &received) == UDP_TRANSPORT_OK);
    CHECK(received == 5 && memcmp(buffer, "reply", 5) == 0);

    /* TCP também acorda sozinho; -1 desabilita sua observação sem fechar o FD. */
    CHECK(send(tcp_client, "q", 1, 0) == 1);
    CHECK(wait_transport(&b) == TCP_TRANSPORT_READABLE);
    CHECK(recv(tcp_server, buffer, 1, 0) == 1 && buffer[0] == 'q');

    CHECK(udp_transport_set_tcp_fd(&b, -1) == UDP_TRANSPORT_OK);
    CHECK(send(tcp_client, "r", 1, 0) == 1);

    struct timeval timeout = {0};
    events = 99;

    CHECK(udp_transport_wait(&b, &timeout, &events) == UDP_TRANSPORT_OK);
    CHECK(events == 0);

    CHECK(udp_transport_set_tcp_fd(&b, tcp_server) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&b) == TCP_TRANSPORT_READABLE);
    CHECK(recv(tcp_server, buffer, 1, 0) == 1 && buffer[0] == 'r');

    CHECK(udp_transport_set_tcp_fd(&b, -2) == UDP_TRANSPORT_ERROR);
    CHECK(udp_transport_set_tcp_fd(&b, FD_SETSIZE) == UDP_TRANSPORT_ERROR);

    /* Zero bytes é um datagrama válido, não fila vazia. */
    CHECK(udp_transport_send(&a, NULL, 0) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&b, NULL, 0, &received) == UDP_TRANSPORT_OK && received == 0);
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received) == UDP_TRANSPORT_ERROR);

    /* Truncamento é erro e não deixa uma cauda como outro datagrama. */
    CHECK(udp_transport_send(&a, "too-long", 8) == UDP_TRANSPORT_OK);
    CHECK(udp_transport_send(&a, "next", 4) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    received = 99;
    CHECK(udp_transport_recv(&b, buffer, 3, &received) == UDP_TRANSPORT_ERROR);
    CHECK(received == 0);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received) == UDP_TRANSPORT_OK);
    CHECK(received == 4 && memcmp(buffer, "next", 4) == 0);

    /* Timeout sem tráfego não inventa um evento de leitura. */
    events = 99;

    CHECK(udp_transport_wait(&b, &timeout, &events) == UDP_TRANSPORT_OK);
    CHECK(events == 0);

    /* Fechar transporte emprestado não fecha o FD do chamador. */
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK);
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK);
    CHECK(a.data == &data_a && data_a.fd == -1);
    CHECK(fcntl(fd_a, F_GETFL) == original_flags);
    CHECK(udp_transport_send(&a, "x", 1) == UDP_TRANSPORT_ERROR);
    CHECK(udp_transport_wait(&a, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);
    CHECK(close(fd_a) == 0);

    /* Socket criado internamente também envia e é liberado no close. */
    config_a.fd = -1;
    config_a.already_bound = 0;
    config_a.local.sin_port = 0;
    CHECK(udp_transport_init_udp(&a, &config_a) == UDP_TRANSPORT_OK);
    CHECK(a.data == &data_a);
    int created_fd = data_a.fd;
    CHECK(udp_transport_send(&a, "new", 3) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received) == UDP_TRANSPORT_OK);
    CHECK(received == 3 && memcmp(buffer, "new", 3) == 0);
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK);
    CHECK(fcntl(created_fd, F_GETFD) == -1 && errno == EBADF);

    /* Falha de init não toma posse do FD externo. */
    config_a.fd = listener;
    config_a.already_bound = 1;
    config_a.take_ownership = 1;
    CHECK(udp_transport_init_udp(&a, &config_a) == UDP_TRANSPORT_ERROR);
    CHECK(a.data == &data_a && data_a.fd == -1);
    CHECK(fcntl(listener, F_GETFD) >= 0);
    CHECK(udp_transport_close(&b) == UDP_TRANSPORT_OK);
    CHECK(fcntl(fd_b, F_GETFD) == -1 && errno == EBADF);
    CHECK(fcntl(tcp_server, F_GETFD) >= 0);
    CHECK(close(tcp_server) == 0);
    CHECK(close(tcp_client) == 0);
    CHECK(close(listener) == 0);
    puts("UDP transport: datagrams, TCP/select, ownership and lifecycle passed");
    return EXIT_SUCCESS;
}
