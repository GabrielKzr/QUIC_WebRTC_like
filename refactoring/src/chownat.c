#include "chownat.h"

static int chownat_init(const struct udp_conn_t* conn) {
    struct chownat_data_t* data = (struct chownat_data_t*)conn->data;
    struct chownat_config_t* config = (struct chownat_config_t*)conn->config;

    if(conn == NULL || data == NULL || config == NULL) return -1;

    struct timeval udp_recv_timeout = {
        .tv_sec = config->udp_recv_timeout_sec,
        .tv_usec = 0
    };

    if(setsockopt(conn->session->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &udp_recv_timeout, sizeof(udp_recv_timeout)) < 0) {
        perror("Erro ao configurar setsockopt\n");
        close(conn->session->socket_fd);
        return -1;
    }

    if(setsockopt(conn->session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &config->reuse, sizeof(config->reuse)) < 0) {
        perror("Erro ao configurar setsockopt\n");
        close(conn->session->socket_fd);
        return -1;
    }    

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

    // depois aqui vai chamar disconnect e fazer mais alguma operação se necessário

    close(conn->session->socket_fd);
    
    if(conn->tcp_tun)
        close(conn->tcp_tun->socket_fd);
    
    DEBUG_PRINT("[DEBUG] chownat_deinit()\n");

    return 0;
}

static int chownat_udp_send_ka(const struct udp_conn_t* conn) {

    int sent = sendto(conn->session->socket_fd, "\0", 1, 0, (struct sockaddr *)&conn->session->dst, sizeof(conn->session->dst));
    if(sent < 0) {
        DEBUG_PRINT("[ERROR] send %s\n", strerror(errno));
        exit(errno);
    }

    DEBUG_PRINT("[DEBUG] Sent keep-alive\n");

    return 0;
} 

static int chownat_hole_punching(const struct udp_conn_t* conn) {

    struct chownat_config_t* config = (struct chownat_config_t*)conn->config;

    if(conn->session->mode == 'c') {

        DEBUG_PRINT("[DEBUG] Opening a connection to the remote end\n"); 

        int attempts = 0;

        while(attempts < config->conn_max_attempts) {

            DEBUG_PRINT("[DEBUG] Attempting to connect\n");

            char* msg = "01\n";

            sendto(conn->session->socket_fd, msg, strlen(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
            static char buffer[4];
            
            if(recv(conn->session->socket_fd, buffer, 3, 0) < 0) { // se socket deu erro ou timeout, tenta novamente
                attempts++;
                continue;
            }

            buffer[3] = 0;

            if(strcmp(buffer, "03\n") == 0) {
                sendto(conn->session->socket_fd, "03\n", strlen(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
                DEBUG_PRINT("[REMOTE] Connection opened to remote end\n");
                return 0;
            }   

            attempts++; // não deve cair aqui, já vou deixar um debug pq provavelmente vai cair
            DEBUG_PRINT("[ERROR] Should not receive the message %s\n", buffer);
        }
    } else if(conn->session->mode == 's') {

        DEBUG_PRINT("[DEBUG] Waiting a connection from the remote end\n"); 
        char buffer[4];
        
        while (1)
        {
            if(recv(conn->session->socket_fd, buffer, 3, 0) < 0) { // se socket deu erro ou timeout, tenta novamente
                chownat_udp_send_ka(conn);
                continue;
            }

            buffer[3] = 0;

            if(strncmp(buffer, "01\n", 3) == 0) {

                DEBUG_PRINT("[REMOTE] Attempted to connect to us, initializing connection\n");

                int attempts = 0; 
                
                while (attempts < config->conn_max_attempts) {
                    
                    DEBUG_PRINT("[DEBUG] Connecting...\n");
                    sendto(conn->session->socket_fd, "03\n", 3, 0, (struct sockaddr *)&conn->session->dst, sizeof(conn->session->dst));
                    
                    if(recv(conn->session->socket_fd, buffer, 3, 0) < 0) {
                        attempts++;
                        continue;
                    }

                    buffer[3] = 0;

                    if(strcmp(buffer, "03\n") == 0) {
                        DEBUG_PRINT("[REMOTE] Connection opened to remote end\n");
                        return 0; // terminou de conectar
                    } else {
                        DEBUG_PRINT("[DEBUG] Should not receive %x. Ignoring\n", buffer[1]);
                    }
                }

                DEBUG_PRINT("[DEBUG] Connection failed\n");
            }

        }
    } else {
        DEBUG_PRINT("[ERROR] mode %c not known\n", conn->session->mode);
        return -1;
    }

    return 0;
}

static int chownat_connect(const struct udp_conn_t* conn) {

    DEBUG_PRINT("[DEBUG] Connected!\n");

    conn->udp_conn_callback(conn, CHOWNAT_UDP_CONNECTED, NULL, 0);

    return 0;
}

static int chownat_disconnect_send(const struct udp_conn_t* conn) {

    DEBUG_PRINT("[DEBUG] Ending a connection with remote end\n"); 

    struct chownat_config_t* config = conn->config;
    struct chownat_data_t* data = conn->data;
    struct tcp_tunneling_t* tcp_tun = conn->tcp_tun;
    char buffer[4];

    int attempts = 0;

    while(attempts < config->conn_max_attempts) {

        DEBUG_PRINT("[DEBUG] Attempting to disconnect\n");

        char* msg = "02\n";

        sendto(conn->session->socket_fd, msg, strlen(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));

        if(recv(conn->session->socket_fd, buffer, 3, 0) < 0) {
            attempts++;
            continue;
        }

        buffer[3] = 0;

        if(strcmp(buffer, "03\n") == 0) {
            sendto(conn->session->socket_fd, "03\n", strlen(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
            break;
        }

        attempts++;
    }

    DEBUG_PRINT("[REMOTE] Connection ended with remote end\n");

    data->id = 0;
    data->expected = 0;
    
    if(tcp_tun) {
        close(tcp_tun->socket_fd);
        tcp_tun->socket_fd = -1;
        close(tcp_tun->accepted_sock);
        tcp_tun->accepted_sock = -1;
    }

    DEBUG_PRINT("[DEBUG] chownat_disconnect()\n");

    return 0;
}

static int chownat_disconnect_recv(const struct udp_conn_t* conn) {

    DEBUG_PRINT("[DEBUG] Attempted to disconnect us, initializing disconnection\n");

    struct chownat_config_t* config = conn->config;
    struct chownat_data_t* data = conn->data;
    struct tcp_tunneling_t* tcp_tun = conn->tcp_tun;
    char buffer[4];

    int attempts = 0;

    while(attempts < config->conn_max_attempts) {

        DEBUG_PRINT("[DEBUG] Disconnecting...\n");

        sendto(conn->session->socket_fd, "03\n", 3, 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));

        if(recv(conn->session->socket_fd, buffer, 3, 0) < 0) {
            attempts++;
            continue;
        }

        buffer[3] = 0;

        if(strcmp(buffer, "03\n") == 0) {
            break;
        } else {
            DEBUG_PRINT("[DEBUG] Should not receive %x. Ignoring\n", buffer[1]);
        }
    }

    DEBUG_PRINT("[REMOTE] Connection ended with remote end\n");

    data->id = 0;
    data->expected = 0;
    
    if(tcp_tun) {
        close(tcp_tun->socket_fd);
        tcp_tun->socket_fd = -1;
        close(tcp_tun->accepted_sock);
        tcp_tun->accepted_sock = -1;
    }    

    DEBUG_PRINT("[DEBUG] chownat_disconnect()\n");

    return 0;
}

static size_t chownat_udp_send(const struct udp_conn_t* conn, void* buf, size_t nbytes) {

    DEBUG_PRINT("[DEBUG] chownat_udp_send()\n");

    if(conn->tcp_tun) // if tcp_tun active, then just use service
        return 0;

    if(nbytes > size-3) // payload é 1021 (outros 3 são header)
        return 0;

    struct chownat_data_t* data = (struct chownat_data_t*)conn->data;

    char* data_in = (char *)buf;

    memcpy(data->buffer, data, nbytes);
    data->sizes[data->id] = nbytes;
    
    char outbuf[size];
    outbuf[0] = '0';
    outbuf[1] = '9';
    outbuf[2] = data->id;

    data->id++;
    if(data->id == 256) data->id = 0;
    
    memcpy(&outbuf[3], data_in, nbytes);
    sendto(conn->session->socket_fd, outbuf, nbytes+3, 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));

    return 1;
}

static size_t chownat_udp_recv(const struct udp_conn_t* conn) {

    static char msg[size];

    struct chownat_data_t* data = conn->data;

    int recvd = recv(conn->session->socket_fd, msg, size, 0);

    if(recvd < 0) {
        DEBUG_PRINT("[ERROR] recv %s\n", strerror(errno));
        return 0; // used mainly when calling on callback
    }    

    else if(recvd < 3) {
        DEBUG_PRINT("[DEBUG] Received keep-alive\n"); // ignore keep-alives
    }

    else if(strncmp(msg, "02\n", 3) == 0) {
        chownat_disconnect_recv(conn);
        return 0; // code for disconnect hole punching defined on udp_conn when calling udp_conn_recv (now on line 240, may be changed, but i think it wont)
    }

    else if(strncmp(msg, "03\n", 3) == 0) {
        DEBUG_PRINT("[DEBUG] handshake"); // mensagem extra que pode acabar vindo em caso de perda de pacote
    }

    else if(strncmp(msg, "08", 2) == 0) { 
        uint8_t got = msg[2];
        DEBUG_PRINT("[DEBUG] Remote host needs packet %d, we're on %d\n", got, data->id);

        for(uint8_t i = got; i < data->id; i++) {
            static char outbuf[size] = {0};
            outbuf[0] = '0';
            outbuf[1] = '9';
            outbuf[2] = i;
            memcpy(&outbuf[3], &data->buffer[i], data->sizes[i]);
            DEBUG_PRINT("[DEBUG] Retransmiting packet %d\n", i);
            sendto(conn->session->socket_fd, outbuf, data->sizes[i], 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
        }

        conn->udp_conn_callback(conn, CHOWNAT_UDP_LOST_DATA, msg, recvd); // return header (there is not data), but data is already retransmited
    }

    else if(strncmp(msg, "09", 2) == 0) {
        uint8_t got = msg[2];
        DEBUG_PRINT("[DEBUG] Got packet %d, expected packet %d\n", got, data->expected);

        if(got != data->expected) {
            char msg[] = "080";
            msg[2] = data->expected;
            sendto(conn->session->socket_fd, msg, sizeof(msg), 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));
        } else if(conn->tcp_tun) {

            DEBUG_PRINT("[DEBUG] Received packet %d\n", got);

            if(send(conn->tcp_tun->accepted_sock, &msg[3], recvd-3, 0) < 0) {
                DEBUG_PRINT("[ERROR] send %s\n", strerror(errno));
                exit(errno);
            }

            data->expected++;
            if(data->expected == 256) data->expected = 0;

            conn->udp_conn_callback(conn, CHOWNAT_UDP_RECV_DATA, &msg[3], recvd-3);
        } else {
            DEBUG_PRINT("[DEBUG] Received packet %d (without tun)\n", got);

            // need to be updated before, because udp_conn_recv can be called inside callback
            data->expected++;
            if(data->expected == 256) data->expected = 0;
            
            // -3 is because of "header", passing just DATA
            conn->udp_conn_callback(conn, CHOWNAT_UDP_RECV_DATA, &msg[3], recvd-3);
        } 
    }

    return recvd;
}

static int chownat_tcp_bind(const struct udp_conn_t* conn) {

    struct tcp_tunneling_t* tcp_tun = conn->tcp_tun;

    if(conn->session->mode == 'c') {

        DEBUG_PRINT("[DEBUG] Binding a new socket to %d\n", tcp_tun->local.sin_port);

        struct timeval tcp_recv_timeout = {
            .tv_sec = tcp_tun->tcp_recv_timeout_sec,
            .tv_usec = 0
        };

        if(setsockopt(tcp_tun->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tcp_recv_timeout, sizeof(tcp_recv_timeout)) < 0) {
            perror("Erro ao configurar setsockopt\n");
            close(tcp_tun->socket_fd);
            return -1;
        }

        if(setsockopt(tcp_tun->socket_fd, SOL_SOCKET, SO_REUSEADDR, &tcp_tun->reuse, sizeof(tcp_tun->reuse)) < 0) {
            perror("Erro ao configurar setsockopt\n");
            close(tcp_tun->socket_fd);
            return -1;
        }    

        if(bind(tcp_tun->socket_fd, (struct sockaddr *)&tcp_tun->local, sizeof(tcp_tun->local)) < 0) {
            perror("Erro ao fazer o bind\n");
            close(tcp_tun->socket_fd);
            return -1;
        }

        if(listen(tcp_tun->socket_fd, 20) < 0) {
            perror("Erro ao escutar\n");
            close(tcp_tun->socket_fd);
            return -1;
        }

        DEBUG_PRINT("[DEBUG] Esperando conexão...\n");

        return 0;

    } else if(conn->session->mode == 's') {

        if(tcp_tun->socket_fd < 0) {
            DEBUG_PRINT("[ERROR] socket %s\n", strerror(errno));
            exit(errno);
        }

        if(connect(tcp_tun->socket_fd, (struct sockaddr*)&tcp_tun->local, sizeof(tcp_tun->local)) < 0) {
            DEBUG_PRINT("[ERROR] connect %s\n", strerror(errno));
            exit(errno);
        }

        tcp_tun->accepted_sock = tcp_tun->socket_fd; // equal file descriptor, server does not have client to be accepted

        DEBUG_PRINT("[DEBUG] connection to local daemon (port %d) opened\n", tcp_tun->local.sin_port);

    } else {
        DEBUG_PRINT("[ERROR] mode %c not known\n", conn->session->mode);
        return -1;
    }

    return 0;
}

static int chownat_tcp_recv(const struct udp_conn_t* conn) {

    static char msg[size-3];

    struct chownat_data_t* data = conn->data;

    int recvd = recv(conn->tcp_tun->accepted_sock, msg, size-3, 0);  

    if(recvd == 0) {
        DEBUG_PRINT("[REMOTE] Attempted to disconnect us\n");
        return -1;
    } else {

        memcpy(&data->buffer[data->id], msg, recvd);
        data->sizes[data->id] = recvd;

        char outbuf[size];
        outbuf[0] = '0';
        outbuf[1] = '9';
        outbuf[2] = data->id;

        data->id++;
        if(data->id == 256) data->id = 0;

        memcpy(&outbuf[3], msg, recvd);
        sendto(conn->session->socket_fd, outbuf, recvd+3, 0, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst));

        conn->udp_conn_callback(conn, CHOWNAT_TCP_DATA_SENT, msg, recvd);
    }

    return 0;
}

struct udp_conn_generic_api_t chownat_api = {
    .init = chownat_init,
    .deinit = chownat_deinit,
    .hole_punching = chownat_hole_punching,
    .connect = chownat_connect,
    .udp_send = chownat_udp_send,
    .udp_recv = chownat_udp_recv,
    .udp_send_ka = chownat_udp_send_ka,
    .disconnect = chownat_disconnect_send,
    .tcp_bind = chownat_tcp_bind,
    .tcp_recv = chownat_tcp_recv
};