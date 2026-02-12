#include "utils.h"
#include "chownat.h"

struct chownat_data_t chownat_data;

struct chownat_config_t chownat_config = {
    .udp_recv_timeout_sec = 1,
    .reuse = 1,
    .conn_max_attempts = 20,
    .dconn_max_attempts = 20
};

int main(int argc, char *argv[]) {
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

    struct udp_conn_t _conn = {
        .name = "conn1",
        .session = &udp_session,
        .config = &chownat_config,
        .data = &chownat_data,
        .api = &chownat_api,
        .udp_conn_callback = 0,
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