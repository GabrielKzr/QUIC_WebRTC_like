#include "utils.h"
#include "ossl_quic.h"

struct ossl_quic_data_t ossl_quic_data;

struct  ossl_quic_config_t ossl_quic_config = {
    .tls13_preset = OSSL_TLS13_PRESET_DEFAULT,
    .tls13_suites = 0,
    .tls13_suites_len = 0,
    .reuse = 1
};

void udp_conn_calback(const struct udp_conn_t* conn, int reason, void* data_in, size_t nbytes) {

}

int app_main(int argc, char *argv[]) {
    int DEBUG, localport, remoteport;
    char *mode, *remoteaddr;
    
    usage(argc, argv, &DEBUG, &mode, &localport, &remoteaddr, &remoteport);
    
    DEBUG_PRINT("[DEBUG] Mode: %s\n", mode);
    DEBUG_PRINT("[DEBUG] Local port: %d\n", localport);
    DEBUG_PRINT("[DEBUG] Remote address: %s\n", remoteaddr);
    DEBUG_PRINT("[DEBUG] Remote port: %d\n", remoteport);
    DEBUG_PRINT("[DEBUG] Debug level: %d\n", DEBUG);

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct udp_conn_session_t udp_session = {
        .socket_fd = sock_fd,
        .mode = mode[1],
        .ka_miss_threshold = 20, // não recebeu keep alive em 20 tentativas
                                 // (aproximadamente 1 minuto e 40 segundos sem receber mensagem ou keep alive)
        .dst = {
            .sin_family = AF_INET,
            .sin_port = htons(remoteport),
            .sin_addr = { .s_addr = inet_addr(remoteaddr) }
        },
        .src = {
            .sin_family = AF_INET,
            .sin_port = htons(remoteport),
            .sin_addr = { .s_addr = INADDR_ANY }
        }
    };

    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

/*
struct tcp_tunneling_t tcp_tun = {
    .socket_fd = tcp_sock,
    .accepted_sock = -1,
    .local = {
        .sin_family = AF_INET,
        .sin_port = htons(localport),
        .sin_addr = {.s_addr = inet_addr(localhost)}
    },
    .reuse = 1
};
*/    
    
    struct udp_conn_t _conn = {
        .name = "conn1",
        .session = &udp_session,
        .config = &ossl_quic_config,
        .data = &ossl_quic_data,
        .api = &ossl_quic_api,
        .udp_conn_callback = udp_conn_calback,
        .tcp_tun = 0
    };

    struct udp_conn_t *conn = &_conn;

    printf("starting init\n");

    udp_conn_init(conn);

    print_sockaddr_in(&conn->session->dst);    

    udp_connection(conn);

    udp_conn_deinit(conn);

    return 0;
}