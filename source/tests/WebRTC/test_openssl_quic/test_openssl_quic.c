#include "utils.h"
#include "ossl_quic.h"

void udp_conn_calback(const udp_conn_t* conn, int reason, void* data_in, size_t nbytes);

ossl_quic_data_t ossl_quic_data = {0};

ossl_quic_config_t ossl_quic_config = {
    .tls13_preset   = OSSL_TLS13_PRESET_DEFAULT,
    .tls13_suites   = 0,
    .tls13_suites_len = 0,
    .reuse          = 1
};

/* ===================== SESSION ===================== */

static udp_session_t udp_session = {0};
static tcp_session_t tcp_session = {0};
static udp_conn_ctrl_t ctrl = {0};

/* ===================== CONTEXT ===================== */

static udp_conn_t _conn = {
    .udp_session          = &udp_session,
    .tcp_session          = &tcp_session,
    .ctrl                 = &ctrl,
    .config               = &ossl_quic_config,
    .data                 = &ossl_quic_data,
    .api                  = &ossl_quic_api,
    .udp_conn_callback    = udp_conn_calback,
};

static udp_conn_t *conn = &_conn;

/* ===================== CALLBACK ===================== */

void udp_conn_calback(const udp_conn_t* conn, int reason, void* data_in, size_t nbytes)
{
    (void)conn;
    (void)reason;
    (void)data_in;
    (void)nbytes;
}

/* ===================== MAIN ===================== */

int main(int argc, char *argv[])
{
    int DEBUG, localport, remoteport;
    char *mode, *remoteaddr;

    usage(argc, argv, &DEBUG, &mode, &localport, &remoteaddr, &remoteport);

    DEBUG_PRINT("[DEBUG] Mode: %s\n", mode);
    DEBUG_PRINT("[DEBUG] Local port: %d\n", localport);
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
    struct sockaddr_in local = {0};

    dst.sin_family = AF_INET;
    dst.sin_port = htons(remoteport);
    dst.sin_addr.s_addr = inet_addr(remoteaddr);

    src.sin_family = AF_INET;
    src.sin_port = htons(localport);
    src.sin_addr.s_addr = INADDR_ANY;

    local.sin_family = AF_INET;
    local.sin_port = htons(localport);
    local.sin_addr.s_addr = inet_addr(localhost);

    /* ===================== SESSION INIT ===================== */

    udp_session.socket_fd = udp_fd;
    udp_session.dst = dst;
    udp_session.src = src;
    udp_session.ka_miss_threshold = 20;

    tcp_session.socket_fd = -1;
    tcp_session.local = local;
    tcp_session.accepted_sock = -1;
    tcp_session.reuse = 1;

    ctrl.mode = *mode;
    ctrl.init = 1;
    FD_ZERO(&ctrl.read_fds);

    /* ===================== INIT FLOW ===================== */

    printf("starting init\n");

    if (udp_conn_init(conn) != 0) {
        fprintf(stderr, "udp_conn_init failed\n");
        exit(EXIT_FAILURE);
    }

    print_sockaddr_in(&conn->udp_session->dst);

    if (udp_connection(conn) != 0) {
        fprintf(stderr, "udp_connection failed\n");
    }

    udp_conn_deinit(conn);

    return 0;
}