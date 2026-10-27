#include "utils.h"
#include "ossl_quic.h"

void udp_conn_calback(const udp_conn_t* conn, int reason, void* data_in, size_t nbytes);

ossl_quic_data_t ossl_quic_data = {0};

ossl_quic_config_t ossl_quic_config = {
    .tls13_preset   = OSSL_TLS13_PRESET_DEFAULT,
    .tls13_suites   = 0,
    .tls13_suites_len = 0,
    .reuse          = 1,
    .cert_file      = "./common/certs/server.crt",
    .key_file       = "./common/certs/server.key"
};

/* ===================== SESSION ===================== */

static udp_session_t udp_session = {0};
static udp_conn_ctrl_t ctrl = {0};

/* ===================== CONTEXT ===================== */

static udp_conn_t _conn = {
    .udp_session          = &udp_session,
    .tcp_session          = NULL,
    .ctrl                 = &ctrl,
    .config               = &ossl_quic_config,
    .data                 = &ossl_quic_data,
    .api                  = &ossl_quic_api,
    .udp_conn_callback    = udp_conn_calback,
};

static udp_conn_t *conn = &_conn;

/* ===================== FINITE QUIC TEST ===================== */

#define TEST_ROUNDS 3

static unsigned completed_rounds = 0;
static int test_failed = 0;
static char received_message[64];
static size_t received_length = 0;

static void fail_test(const udp_conn_t* conn, const char* reason)
{
    fprintf(stderr, "QUIC UDP test failed: %s\n", reason);
    test_failed = 1;
    udp_conn_disconnect(conn);
}

static int send_message(const udp_conn_t* conn, const char* kind, unsigned round)
{
    char message[64];
    int length = snprintf(message, sizeof(message), "QUIC %s %u\n", kind, round);
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
    if (reason == OSSL_QUIC_UDP_CONNECTED) {
        if (conn->ctrl->mode == 'c')
            send_message(conn, "request", 0);
        return;
    }
    if (reason != OSSL_QUIC_UDP_RECV_DATA)
        return;
    if (data_in == NULL || nbytes == 0) {
        fail_test(conn, "empty payload");
        return;
    }

    // QUIC entrega bytes de um stream, sem preservar fronteiras de mensagens.
    const char* bytes = data_in;
    for (size_t i = 0; i < nbytes; i++) {
        if (received_length == sizeof(received_message)) {
            fail_test(conn, "message too long");
            return;
        }
        received_message[received_length++] = bytes[i];
        if (bytes[i] != '\n')
            continue;

        char expected[64];
        const char* kind = conn->ctrl->mode == 'c' ? "response" : "request";
        int length = snprintf(expected, sizeof(expected), "QUIC %s %u\n", kind, completed_rounds);
        if (completed_rounds >= TEST_ROUNDS || length < 0 ||
            (size_t)length >= sizeof(expected) || received_length != (size_t)length ||
            memcmp(received_message, expected, received_length) != 0) {
            fail_test(conn, "unexpected payload or sequence");
            return;
        }
        received_length = 0;
        if (conn->ctrl->mode == 's' &&
            !send_message(conn, "response", completed_rounds))
            return;

        completed_rounds++;
        DEBUG_PRINT("[DEBUG] Validated QUIC round %u/%u\n", completed_rounds, TEST_ROUNDS);
        if (conn->ctrl->mode == 'c') {
            if (completed_rounds < TEST_ROUNDS) {
                if (!send_message(conn, "request", completed_rounds))
                    return;
            } else {
                if (i + 1 != nbytes) {
                    fail_test(conn, "extra data after final response");
                    return;
                }
                if (udp_conn_disconnect(conn) < 0) {
                    fprintf(stderr, "QUIC UDP test failed: disconnect failed\n");
                    test_failed = 1;
                }
                return;
            }
        }
    }
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
        perror("socket");
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
    if (status != UDP_CONN_OK || completed_rounds != TEST_ROUNDS ||
        received_length != 0 || !ossl_quic_data.closed) {
        fprintf(stderr, "QUIC UDP test incomplete: status=%d rounds=%u/%u pending=%zu\n",
                status, completed_rounds, TEST_ROUNDS, received_length);
        test_failed = 1;
    }
    if (udp_conn_deinit(conn) != UDP_CONN_OK)
        test_failed = 1;

    if (test_failed)
        return EXIT_FAILURE;
    printf("QUIC UDP test passed: %u rounds validated and session closed\n", completed_rounds);
    return EXIT_SUCCESS;
}
