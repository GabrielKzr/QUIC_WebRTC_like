#include "utils.h"
#include "chownat.h"

struct chownat_data_t chownat_data;

struct chownat_config_t chownat_config = {
    .udp_recv_timeout_sec = 1,
    .reuse = 1,
    .conn_max_attempts = 20,
    .dconn_max_attempts = 20
};

void udp_conn_calback(const struct udp_conn_t* conn, int reason, void* data_in, size_t nbytes) {

    switch (reason)
    {
    case CHOWNAT_UDP_CONNECTED :
    { 
        DEBUG_PRINT("[AAAAAAAAAAAAAAAAAAAAAAAAAAAAA]\n");
        DEBUG_PRINT("[DEBUG] Connected Successfuly\n");
        
        char *msg = "Hello World!";
        // sendto(conn->session->socket_fd, msg, strlen(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
        udp_conn_send(conn, msg, strlen(msg));
        
        break;
    }

    case CHOWNAT_UDP_RECV_DATA:
    {
        char *msg = (char *)data_in;
        
        DEBUG_PRINT("[DEBUG] Received Data [%s]\n", msg);
        
        size_t send_bytes = 0;
        struct chownat_data_t* data = (struct chownat_data_t*)conn->data;
        
        char buf[64];
        memset(buf, 0, 64);
        
        if(conn->session->mode == 'c') {
            send_bytes = sprintf(buf, "%d: Hello from client", data->expected);
            // sendto(conn->session->socket_fd, buf, send_bytes, 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
            udp_conn_send(conn, buf, send_bytes);
        } else {
            send_bytes = sprintf(buf, "%d: Hello from server", data->expected);
            // sendto(conn->session->socket_fd, buf, send_bytes, 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
            udp_conn_send(conn, buf, send_bytes);
        } 
        
        sleep(1);
        
        break;
    }

    case CHOWNAT_UDP_LOST_DATA:

        DEBUG_PRINT("[DEBUG] Lost Data\n");

        break;
    
    case CHOWNAT_TCP_DATA_SENT:

        DEBUG_PRINT("[DEBUG] TCP Data Sent");

        break;

    default:

        DEBUG_PRINT("[DEBUG] Unknown Reason\n");
        break;
    }
}


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

    struct udp_conn_t _conn = {
        .name = "conn1",
        .session = &udp_session,
        .config = &chownat_config,
        .data = &chownat_data,
        .api = &chownat_api,
        .udp_conn_callback = udp_conn_calback,
        .tcp_tun = &tcp_tun
    };

    struct udp_conn_t *conn = &_conn;

    printf("starting init\n");

    udp_conn_init(conn);

    print_sockaddr_in(&conn->session->dst);    

    udp_connection(conn);

    udp_conn_deinit(conn);

    return 0;
}