#include "chownat.h"

static int chownat_init(const struct udp_conn_t* conn) {

    struct chownat_data_t* data = (struct chownat_data_t*)conn->data;

    if(bind(conn->session->socket_fd, (struct sockaddr*)&conn->session->src, sizeof(conn->session->src)) < 0) {
        printf("[ERROR] bind %s\n", strerror(errno));
        return 0;
    }  
    
    data->id = 0;
    data->expected = 0;
    data->busy = 1;
    memset(data->buffer, 0, sizeof(data->buffer));
    memset(data->sizes, 0, sizeof(data->sizes));

    DEBUG_PRINT("[DEBUG] chownat_init()\n");

    return 0;
}

static int chownat_deinit(const struct udp_conn_t* conn) {

    DEBUG_PRINT("[DEBUG] chownat_deinit()");

    return 0;
}

static int chownat_hole_punching(const struct udp_conn_t* conn) {

    if(conn->session->mode == 'c') {

       DEBUG_PRINT("[DEBUG] Opening a connection to the remote end\n"); 

        while(1) {
            DEBUG_PRINT("[DEBUG] Attempting to connect\n");

            char* msg = "01\n";

            sendto(conn->session->socket_fd, msg, strlen(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
            static char buffer[4];
            recv(conn->session->socket_fd, buffer, 3, 0);
            buffer[4] = 0;

            if(strcmp(buffer, "03\n") == 0) {
                sendto(conn->session->socket_fd, "03\n", strlen(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
                DEBUG_PRINT("[REMOTE] Connection opened to remote end\n");
                return 0;
            }
        }
    } else if(conn->session->mode == 's') {




    } else {
        DEBUG_PRINT("[ERROR] mode %c not known", conn->session->mode);
        return -1;
    }

    return 0;
}

static int chownat_connect(const struct udp_conn_t* conn) {

    return 0;
}

static size_t chownat_udp_send(const struct udp_conn_t* conn, void* buf) {

    return 0;
}

static size_t chownat_udp_recv(const struct udp_conn_t* conn) {

    return 0;
}

static int chownat_udp_send_ka(const struct udp_conn_t* conn) {

    int sent = sendto(conn->session->socket_fd, "\0", 1, 0, (struct sockaddr *)&conn->session->dst, sizeof(conn->session->dst));
    if(sent < 0) {
        DEBUG_PRINT("[ERROR] send %s\n", strerror(errno));
        exit(errno);
    }

    DEBUG_PRINT("[DEBUG] Sent keep-alive");

    return 0;
} 

static int chownat_disconnect(const struct udp_conn_t* conn, struct timeval*) {

    return 0;
}

static int chownat_tcp_bind(const struct udp_conn_t* conn) {

    struct tcp_tunneling_t* tcp_tun = conn->tcp_tun;

    if(conn->session->mode == 'c') {

        DEBUG_PRINT("[DEBUG] Binding a new socket to %d\n", tcp_tun->local.sin_port);

        if(setsockopt(tcp_tun->socket_fd, SOL_SOCKET, SO_REUSEADDR, &tcp_tun->reuse, sizeof(tcp_tun->reuse)) < 0) {
            perror("Erro ao configurar setsockopt");
            close(tcp_tun->socket_fd);
            return -1;
        }    

        if(bind(tcp_tun->socket_fd, (struct sockaddr *)&tcp_tun->local, sizeof(tcp_tun->local)) < 0) {
            perror("Erro ao fazer o bind");
            close(tcp_tun->socket_fd);
            return -1;
        }

        if(listen(tcp_tun->socket_fd, 20) < 0) {
            perror("Erro ao escutar");
            close(tcp_tun->socket_fd);
            return -1;
        }

        DEBUG_PRINT("[DEBUG] Esperando conexão...\n");

        return 0;

    } else if(conn->session->mode == 's') {

        close(tcp_tun->socket_fd); // garante que a socket vai estar fechada
        tcp_tun->socket_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(tcp_tun->socket_fd < 0) {
            DEBUG_PRINT("[ERROR] socket %s\n", strerror(errno));
            exit(errno);
        }

        if(connect(tcp_tun->socket_fd, (struct sockaddr*)&tcp_tun->local, sizeof(tcp_tun->local)) < 0) {
            DEBUG_PRINT("[ERROR] connect %s\n", strerror(errno));
            exit(errno);
        }

        DEBUG_PRINT("[DEBUG] connection to local daemon (port %d) opened\n", tcp_tun->local.sin_port);

    } else {
        DEBUG_PRINT("[ERROR] mode %c not known", conn->session->mode);
        return -1;
    }

    return 0;
}

static int chownat_tcp_recv(const struct udp_conn_t* conn) {

    return 0;
}

const struct udp_conn_generic_api_t chownat_api = {
    .init = chownat_init,
    .deinit = chownat_deinit,
    .hole_punching = chownat_hole_punching,
    .connect = chownat_connect,
    .udp_send = chownat_udp_send,
    .udp_recv = chownat_udp_recv,
    .udp_send_ka = chownat_udp_send_ka,
    .disconnect = chownat_disconnect,
    .tcp_bind = chownat_tcp_bind,
    .tcp_recv = chownat_tcp_recv
};