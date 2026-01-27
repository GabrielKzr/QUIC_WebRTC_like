#include <udp_conn.h>

uint8_t udp_conn_init(struct udp_conn_t *conn, int socket_fd, char mode, int remoteport, char* remoteaddr) {

    conn->socket_fd = socket_fd;
    conn->mode = mode;
    
    // conn->dst = {};
    conn->dst.sin_family = AF_INET;
    conn->dst.sin_port = htons(remoteport);
    conn->dst.sin_addr.s_addr = inet_addr(remoteaddr);
    
    // conn->src = {};
    conn->src.sin_family = AF_INET;
    conn->src.sin_port = htons(remoteport);
    conn->src.sin_addr.s_addr = INADDR_ANY;    

    if(bind(conn->socket_fd, (struct sockaddr*)&conn->src, sizeof(conn->src)) < 0) {
        printf("ERROR: bind %s\n", strerror(errno));
        return 0;
    }

    return 1;
}

static hole_punching(struct udp_conn_t *conn) {

    switch (conn->mode)
    {
    case 'c':
        
        break;
    
    case 's':

        break;

    default:
        break;
    }

}

static uint8_t udp_conn_connect(struct udp_conn_t *conn) {

    hole_punching(conn);

    DEBUG_PRINT("[DEBUG] Remote connected\n");


}

static uint8_t udp_conn_send(struct udp_conn_t *conn, void *data) {


}

static uint8_t udp_conn_recv(struct udp_conn_t *conn, void *data) {


}

static uint8_t udp_conn_disconnect(struct udp_conn_t *conn) {


}