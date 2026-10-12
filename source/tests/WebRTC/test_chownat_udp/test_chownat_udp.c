#include "utils.h"
#include "chownat.h"

void udp_conn_calback(const udp_conn_t* conn, int reason, void* data_in, size_t nbytes);

chownat_data_t chownat_data = {0};

chownat_config_t chownat_config = {
    .udp_recv_timeout_sec = 1,
    .reuse = 1,
    .conn_max_attempts = 20,
    .dconn_max_attempts = 20
};

/* ===================== SESSION ===================== */

static udp_session_t udp_session = {0};
// static tcp_session_t tcp_session = {0};
static udp_conn_ctrl_t ctrl = {0};

/* ===================== CONTEXT ===================== */

static udp_conn_t _conn = {
    .udp_session = &udp_session,
    .tcp_session = NULL,
    .ctrl = &ctrl,
    .config = &chownat_config,
    .data = &chownat_data,
    .api = &chownat_api,
    .udp_conn_callback = udp_conn_calback
};

static udp_conn_t *conn = &_conn;

/* ===================== FINITE UDP TEST ===================== */

#define TEST_ROUNDS 3

static unsigned completed_rounds = 0;
static int test_failed = 0;

static void fail_test(const udp_conn_t* conn, const char* reason)
{
    fprintf(stderr, "UDP test failed: %s\n", reason);
    test_failed = 1;
    udp_conn_disconnect(conn);
}

static int send_message(const udp_conn_t* conn, const char* kind, unsigned round)
{
    char message[64];
    int length = snprintf(message, sizeof(message), "UDP %s %u", kind, round);
    size_t sent = 0;

    if (length < 0 || (size_t)length >= sizeof(message) ||
        udp_conn_send(conn, message, (size_t)length, &sent) < 0 ||
        sent != (size_t)length) {
        fail_test(conn, "could not send complete message");
        return 0;
    }
    return 1;
}

void udp_conn_calback(const udp_conn_t* conn, int reason, void* data_in, size_t nbytes)
{
    if (test_failed || conn->tcp_session)
        return;

    if (reason == CHOWNAT_UDP_CONNECTED) {
        // Apenas o cliente inicia; servidor responde a cada mensagem validada.
        if (conn->ctrl->mode == 'c')
            send_message(conn, "request", 0);
        return;
    }

    if (reason != CHOWNAT_UDP_RECV_DATA)
        return;

    char expected[64];
    const char* kind = conn->ctrl->mode == 'c' ? "response" : "request";
    int length = snprintf(expected, sizeof(expected), "UDP %s %u", kind, completed_rounds);
    if (completed_rounds >= TEST_ROUNDS || length < 0 ||
        (size_t)length >= sizeof(expected) || data_in == NULL ||
        nbytes != (size_t)length || memcmp(data_in, expected, nbytes) != 0) {
        fail_test(conn, "unexpected payload or sequence");
        return;
    }

    if (conn->ctrl->mode == 's' &&
        !send_message(conn, "response", completed_rounds))
        return;

    completed_rounds++;
    DEBUG_PRINT("[DEBUG] Validated UDP round %u/%u\n", completed_rounds, TEST_ROUNDS);

    if (conn->ctrl->mode == 'c') {
        if (completed_rounds < TEST_ROUNDS) {
            send_message(conn, "request", completed_rounds);
        } else if (udp_conn_disconnect(conn) < 0) {
            test_failed = 1;
            fprintf(stderr, "UDP test failed: disconnect failed\n");
        }
    }
    // O servidor aguarda o disconnect do cliente antes de terminar.
}

/* ===================== MAIN ===================== */

int main(int argc, char *argv[])
{
    int DEBUG, remoteport;
    char *mode, *remoteaddr;

    usage(argc, argv, &DEBUG, &mode, NULL, &remoteaddr, &remoteport);

    DEBUG_PRINT("[DEBUG] Mode: %s\n", mode);
    DEBUG_PRINT("[DEBUG] Remote address: %s\n", remoteaddr);
    DEBUG_PRINT("[DEBUG] Remote port: %d\n", remoteport);
    DEBUG_PRINT("[DEBUG] Debug level: %d\n", DEBUG);

    /* ===================== SOCKETS ===================== */

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        perror("udp socket");
        exit(EXIT_FAILURE);
    }

    /* ===================== ADDR CONFIG ===================== */

    struct sockaddr_in dst = {0};
    struct sockaddr_in src = {0};

    dst.sin_family = AF_INET;
    dst.sin_port = htons(remoteport);
    dst.sin_addr.s_addr = inet_addr(remoteaddr);

    src.sin_family = AF_INET;
    src.sin_port = htons(remoteport);
    src.sin_addr.s_addr = INADDR_ANY;

    /* ===================== SESSION INIT ===================== */

    udp_session.socket_fd = udp_fd;
    udp_session.dst = dst;
    udp_session.src = src;
    udp_session.ka_miss_threshold = 20;

    ctrl.mode = mode[1];
    ctrl.init = 0;
    FD_ZERO(&ctrl.read_fds);

    /* ===================== INIT FLOW ===================== */

    DEBUG_PRINT("starting init\n");

    if (udp_conn_init(conn) != 0) {
        fprintf(stderr, "udp_conn_init failed\n");
        exit(EXIT_FAILURE);
    }

    print_sockaddr_in(&conn->udp_session->dst);

    udp_conn_status_t status = udp_connection(conn);
    if (status != UDP_CONN_OK) {
        fprintf(stderr, "udp_connection failed: %d\n", status);
        test_failed = 1;
    }

    if (completed_rounds != TEST_ROUNDS || !chownat_data.closed) {
        fprintf(stderr, "UDP test incomplete: %u/%u rounds\n", completed_rounds, TEST_ROUNDS);
        test_failed = 1;
    }
    if (udp_conn_deinit(conn) != UDP_CONN_OK)
        test_failed = 1;

    if (test_failed)
        return EXIT_FAILURE;

    printf("UDP test passed: %u rounds validated and session closed\n", completed_rounds);
    return EXIT_SUCCESS;
}