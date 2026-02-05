#include "utils.h"
#include "udp_conn.h"

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
        .mode = mode[0],
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
        .config = 0,
        .data = 0,
        .reasons = 0,
        .api = 0,
        .udp_conn_callback = 0,
        .tcp_tun = 0
    };

    struct udp_conn_t *conn = &_conn;

    udp_conn_init(conn);

    print_sockaddr_in(&conn->session->dst);    


    return 0;
}