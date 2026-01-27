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

    static struct udp_conn_t _conn = {};
    static struct udp_conn_t *conn = &_conn;

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    udp_conn_init(conn, sock_fd, mode[1], remoteport, remoteaddr);

    print_sockaddr_in(&conn->dst);    


    return 0;
}