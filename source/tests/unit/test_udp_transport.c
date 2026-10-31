#define _POSIX_C_SOURCE 200809L
#include "direct_udp_transport.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
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

/* Backend de teste: controle no único socket UDP e timers sem payload bruto. */
typedef struct {
    unsigned consumed;
    unsigned queries;
    unsigned dispatches;
    unsigned timers;
    unsigned payload_after;
    int buffered;
    int fail_dispatch;
    int fail_timeout;
    int invalid_timeout;
    int64_t next_timer;
} wait_backend_t;

static int64_t test_now_us(void) {
    struct timespec now;
    CHECK(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
    return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

static udp_transport_status_t test_get_timeout(udp_transport_t *transport, struct timeval *timeout, int *has_timeout) {
    wait_backend_t *data = transport->data;
    ++data->queries;
    if (data->fail_timeout) return UDP_TRANSPORT_ERROR;

    int64_t remaining = data->next_timer - test_now_us();
    if (remaining < 0 || data->buffered) remaining = 0;
    *has_timeout = 1;
    *timeout = (struct timeval){.tv_sec = remaining / 1000000,
                               .tv_usec = remaining % 1000000};
    if (data->invalid_timeout) timeout->tv_usec = 1000000;
    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t test_dispatch(udp_transport_t *transport, udp_transport_wait_t *wait) {
    wait_backend_t *data = transport->data;
    ++data->dispatches;
    if (data->fail_dispatch) return UDP_TRANSPORT_ERROR;

    if (FD_ISSET(transport->udp_fd, &wait->read_fds)) {
        char control;
        CHECK(recv(transport->udp_fd, &control, 1, MSG_DONTWAIT) == 1);
        ++data->consumed;
    }

    if (transport->tcp_fd >= 0 && FD_ISSET(transport->tcp_fd, &wait->read_fds)) {
        wait->events |= TCP_TRANSPORT_READABLE;
    }

    int64_t now = test_now_us();
    if (now >= data->next_timer) {
        ++data->timers;
        data->next_timer = now + 2000;
    }
    if (data->payload_after && data->timers >= data->payload_after) {
        data->buffered = 1;
    }
    if (data->buffered) wait->events |= UDP_TRANSPORT_READABLE;

    return UDP_TRANSPORT_OK;
}

static void test_internal_wait(void) {
    struct sockaddr_in udp_address;
    int udp_fd = bound_socket(SOCK_DGRAM, &udp_address);
    int sender = socket(AF_INET, SOCK_DGRAM, 0);
    int tcp[2];
    CHECK(sender >= 0 && pipe(tcp) == 0 && tcp[0] < FD_SETSIZE);

    wait_backend_t data = {
        .next_timer = test_now_us() + 1000000
    };
    const udp_transport_ops_t ops = {
        .get_timeout = test_get_timeout, .dispatch = test_dispatch
    };
    udp_transport_t transport = {
        .ops = &ops, .opened = 1, .udp_fd = udp_fd, .tcp_fd = -1, .data = &data
    };
    for (int i = 0; i < 2; ++i) {
        CHECK(sendto(sender, "c", 1, 0, (struct sockaddr *)&udp_address,
                     sizeof(udp_address)) == 1);
    }
    unsigned events = 99;

    /* Controle sem payload devolve o comando após um único select/dispatch. */
    for (unsigned i = 1; i <= 2; ++i) {
        CHECK(udp_transport_wait(&transport, NULL, &events) == UDP_TRANSPORT_OK);
        CHECK(events == 0 && data.consumed == i);
        CHECK(data.queries == i && data.dispatches == i);
    }

    /* Timer interno também retorna sem payload; o chamador faz a próxima rodada. */
    data.payload_after = 2;
    data.timers = 0;
    data.next_timer = test_now_us() + 3000;
    CHECK(udp_transport_wait(&transport, NULL, &events) == UDP_TRANSPORT_OK);
    CHECK(events == 0 && data.timers == 1);
    CHECK(data.queries == 3 && data.dispatches == 3);
    CHECK(udp_transport_wait(&transport, NULL, &events) == UDP_TRANSPORT_OK);
    CHECK(events == UDP_TRANSPORT_READABLE && data.timers == 2);
    CHECK(data.queries == 4 && data.dispatches == 4);

    /* Payload já disponível não bloqueia, e TCP é reportado na mesma chamada. */
    transport.tcp_fd = tcp[0];
    CHECK(write(tcp[1], "t", 1) == 1);
    CHECK(udp_transport_wait(&transport, NULL, &events) == UDP_TRANSPORT_OK);
    CHECK(events == (UDP_TRANSPORT_READABLE | TCP_TRANSPORT_READABLE));
    char byte;
    CHECK(read(tcp[0], &byte, 1) == 1);

    /* Prazo do chamador menor que o timer interno, com argumento preservado. */
    data.buffered = 0;
    data.payload_after = 0;
    data.timers = 0;
    data.next_timer = test_now_us() + 1000000;
    struct timeval timeout = {.tv_usec = 3000};
    int64_t started = test_now_us();
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_OK);
    CHECK(events == 0 && data.timers == 0);
    CHECK(test_now_us() - started >= 3000);
    CHECK(timeout.tv_sec == 0 && timeout.tv_usec == 3000);

    /* Timer do transporte menor que o prazo do chamador, em chamadas distintas. */
    for (int i = 0; i < 2; ++i) {
        data.buffered = 0;
        data.timers = 0;
        data.payload_after = 1;
        data.next_timer = test_now_us() + 3000;
        timeout = (struct timeval){.tv_usec = 50000};
        CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_OK);
        CHECK(events == UDP_TRANSPORT_READABLE && data.timers == 1);
        CHECK(timeout.tv_sec == 0 && timeout.tv_usec == 50000);
    }
    data.buffered = 0;
    data.payload_after = 0;
    data.next_timer = test_now_us() + 1000000;

    data.fail_timeout = 1;
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);
    data.fail_timeout = 0;
    data.invalid_timeout = 1;
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);
    data.invalid_timeout = 0;

    timeout = (struct timeval){0};
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_OK);
    CHECK(events == 0);
    transport.udp_fd = -1;
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_ERROR);
    transport.udp_fd = FD_SETSIZE;
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_ERROR);
    transport.udp_fd = udp_fd;
    data.fail_dispatch = 1;
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);
    timeout.tv_usec = 1000000;
    CHECK(udp_transport_wait(&transport, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);

    CHECK(close(udp_fd) == 0 && close(sender) == 0);
    for (int i = 0; i < 2; ++i) {
        CHECK(close(tcp[i]) == 0);
    }
}

int main(void) {
    test_internal_wait();
    struct sockaddr_in address_a, address_b;
    int fd_a = bound_socket(SOCK_DGRAM, &address_a);
    int fd_b = bound_socket(SOCK_DGRAM, &address_b);
    int original_flags = fcntl(fd_a, F_GETFL);
    CHECK(original_flags >= 0);
    static direct_udp_transport_data_t data_a, data_b;
    udp_transport_t a = {.ops = &direct_ops, .data = &data_a};
    udp_transport_t b = {.ops = &direct_ops, .data = &data_b};
    udp_transport_config_t common_a = {.udp_fd = fd_a, .tcp_fd = -1};
    udp_transport_config_t common_b = {.udp_fd = fd_b, .tcp_fd = -1};
    direct_udp_transport_config_t config_a = {
        .already_bound = 1,
        .local = address_a, .peer = address_b
    };
    direct_udp_transport_config_t config_b = {
        .already_bound = 1, .take_ownership = 1,
        .local = address_b, .peer = address_a
    };
    a.transport_config = &common_a; a.config = &config_a;
    b.transport_config = &common_b; b.config = &config_b;
    udp_transport_t without_storage = {.ops = &direct_ops, .transport_config = &common_a, .config = &config_a};
    CHECK(udp_transport_open(&without_storage) == UDP_TRANSPORT_ERROR);
    CHECK(fcntl(fd_a, F_GETFD) >= 0);
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK); /* Dados static ainda zerados. */
    CHECK(a.data == &data_a && fcntl(fd_a, F_GETFD) >= 0);
    CHECK(udp_transport_open(&a) == UDP_TRANSPORT_OK);
    CHECK(udp_transport_open(&b) == UDP_TRANSPORT_OK);
    CHECK(a.tcp_fd == -1 && a.opened && a.ops == &direct_ops);
    CHECK(udp_transport_open(&a) == UDP_TRANSPORT_ERROR);

    char buffer[32];
    const struct timeval no_wait = {0};
    const struct timeval one_second = {.tv_sec = 1};
    size_t received = 99;
    CHECK(udp_transport_recv(&a, buffer, sizeof(buffer), &received, &no_wait) == UDP_TRANSPORT_ERROR);
    CHECK(received == 0 && fcntl(fd_a, F_GETFL) == original_flags);

    int64_t started = test_now_us();
    received = 99;
    CHECK(udp_transport_recv(&a, buffer, sizeof(buffer), &received, &one_second) == UDP_TRANSPORT_ERROR);
    CHECK(received == 0 && test_now_us() - started >= 1000000);
    CHECK(one_second.tv_sec == 1 && one_second.tv_usec == 0);
    CHECK(a.tcp_fd == -1 && fcntl(fd_a, F_GETFL) == original_flags);

    const struct timeval invalid_timeout = {.tv_usec = 1000000};
    received = 99;
    CHECK(udp_transport_recv(&a, buffer, sizeof(buffer), &received, &invalid_timeout) == UDP_TRANSPORT_ERROR);
    CHECK(received == 0 && a.tcp_fd == -1);
    CHECK(udp_transport_send(&a, "invalid", 7, &invalid_timeout) == UDP_TRANSPORT_ERROR);

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
    CHECK(udp_transport_send(&a, payload, sizeof(payload), &one_second) == UDP_TRANSPORT_OK);
    CHECK(one_second.tv_sec == 1 && one_second.tv_usec == 0);
    CHECK(send(tcp_client, "t", 1, 0) == 1);

    b.tcp_fd = tcp_server;

    unsigned events;

    events = wait_transport(&b);
    CHECK(events == (UDP_TRANSPORT_READABLE | TCP_TRANSPORT_READABLE));

    CHECK(recv(tcp_server, buffer, 1, 0) == 1 && buffer[0] == 't');
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received, &no_wait) == UDP_TRANSPORT_OK);
    CHECK(received == sizeof(payload) && memcmp(buffer, payload, received) == 0);
    CHECK(udp_transport_send(&b, "reply", 5, NULL) == UDP_TRANSPORT_OK);
    CHECK(b.tcp_fd == tcp_server);
    CHECK(wait_transport(&a) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&a, buffer, sizeof(buffer), &received, NULL) == UDP_TRANSPORT_OK);
    CHECK(received == 5 && memcmp(buffer, "reply", 5) == 0);

    /* TCP também acorda sozinho; -1 desabilita sua observação sem fechar o FD. */
    CHECK(send(tcp_client, "q", 1, 0) == 1);
    received = 99;
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received, &one_second) == UDP_TRANSPORT_ERROR);
    CHECK(received == 0 && b.tcp_fd == tcp_server);
    CHECK(one_second.tv_sec == 1 && one_second.tv_usec == 0);
    CHECK(wait_transport(&b) == TCP_TRANSPORT_READABLE);
    CHECK(recv(tcp_server, buffer, 1, 0) == 1 && buffer[0] == 'q');

    b.tcp_fd = -1;
    CHECK(send(tcp_client, "r", 1, 0) == 1);

    struct timeval timeout = {0};
    events = 99;

    CHECK(udp_transport_wait(&b, &timeout, &events) == UDP_TRANSPORT_OK);
    CHECK(events == 0);

    b.tcp_fd = tcp_server;
    CHECK(wait_transport(&b) == TCP_TRANSPORT_READABLE);
    CHECK(recv(tcp_server, buffer, 1, 0) == 1 && buffer[0] == 'r');

    b.tcp_fd = -2;
    CHECK(udp_transport_wait(&b, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);
    b.tcp_fd = FD_SETSIZE;
    CHECK(udp_transport_wait(&b, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);
    b.tcp_fd = tcp_server;

    /* Zero bytes é um datagrama válido, não fila vazia. */
    CHECK(udp_transport_send(&a, NULL, 0, &no_wait) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&b, NULL, 0, &received, &no_wait) == UDP_TRANSPORT_OK && received == 0);
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received, &no_wait) == UDP_TRANSPORT_ERROR);

    /* Truncamento é erro e não deixa uma cauda como outro datagrama. */
    CHECK(udp_transport_send(&a, "too-long", 8, &no_wait) == UDP_TRANSPORT_OK);
    CHECK(udp_transport_send(&a, "next", 4, &no_wait) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    received = 99;
    CHECK(udp_transport_recv(&b, buffer, 3, &received, &no_wait) == UDP_TRANSPORT_ERROR);
    CHECK(received == 0);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received, &no_wait) == UDP_TRANSPORT_OK);
    CHECK(received == 4 && memcmp(buffer, "next", 4) == 0);

    /* Timeout sem tráfego não inventa um evento de leitura. */
    events = 99;

    CHECK(udp_transport_wait(&b, &timeout, &events) == UDP_TRANSPORT_OK);
    CHECK(events == 0);

    /* Fechar transporte emprestado não fecha o FD do chamador. */
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK);
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK);
    CHECK(a.data == &data_a && a.udp_fd == -1);
    CHECK(!a.opened && a.ops == &direct_ops);
    CHECK(fcntl(fd_a, F_GETFL) == original_flags);
    CHECK(udp_transport_send(&a, "x", 1, &no_wait) == UDP_TRANSPORT_ERROR);
    CHECK(udp_transport_wait(&a, &timeout, &events) == UDP_TRANSPORT_ERROR);
    CHECK(events == 0);
    CHECK(close(fd_a) == 0);

    /* Socket criado internamente também envia e é liberado no close. */
    common_a.udp_fd = -1;
    config_a.already_bound = 0;
    config_a.local.sin_port = 0;
    CHECK(udp_transport_open(&a) == UDP_TRANSPORT_OK);
    CHECK(a.data == &data_a);
    int created_fd = a.udp_fd;
    CHECK(udp_transport_send(&a, "new", 3, &no_wait) == UDP_TRANSPORT_OK);
    CHECK(wait_transport(&b) & UDP_TRANSPORT_READABLE);
    CHECK(udp_transport_recv(&b, buffer, sizeof(buffer), &received, &no_wait) == UDP_TRANSPORT_OK);
    CHECK(received == 3 && memcmp(buffer, "new", 3) == 0);
    CHECK(udp_transport_close(&a) == UDP_TRANSPORT_OK);
    CHECK(fcntl(created_fd, F_GETFD) == -1 && errno == EBADF);

    /* Falha de init não toma posse do FD externo. */
    common_a.udp_fd = listener;
    config_a.already_bound = 1;
    config_a.take_ownership = 1;
    CHECK(udp_transport_open(&a) == UDP_TRANSPORT_ERROR);
    CHECK(a.data == &data_a && a.udp_fd == -1);
    CHECK(!a.opened && a.ops == &direct_ops);
    CHECK(fcntl(listener, F_GETFD) >= 0);
    CHECK(udp_transport_close(&b) == UDP_TRANSPORT_OK);
    CHECK(fcntl(fd_b, F_GETFD) == -1 && errno == EBADF);
    CHECK(fcntl(tcp_server, F_GETFD) >= 0);
    CHECK(close(tcp_server) == 0);
    CHECK(close(tcp_client) == 0);
    CHECK(close(listener) == 0);
    puts("UDP transport: datagrams, shared wait, internal controls/timers and lifecycle passed");
    return EXIT_SUCCESS;
}
