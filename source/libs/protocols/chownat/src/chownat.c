#include "chownat.h"
#include "utils.h"

static udp_conn_status_t chownat_init(const udp_conn_t* conn);
static udp_conn_status_t chownat_deinit(const udp_conn_t* conn);
static udp_conn_status_t chownat_is_closed(const udp_conn_t* conn);
static udp_conn_status_t chownat_udp_send_ka(const udp_conn_t* conn);
static udp_conn_status_t chownat_hole_punching(const udp_conn_t* conn);
static udp_conn_status_t chownat_connect(const udp_conn_t* conn);
static udp_conn_status_t chownat_disconnect_send(const udp_conn_t* conn);
static udp_conn_status_t chownat_disconnect_recv(const udp_conn_t* conn);
static udp_conn_status_t chownat_udp_send(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *sent_bytes);
static udp_conn_status_t chownat_udp_recv(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *recv_bytes);

static udp_conn_status_t chownat_init(const udp_conn_t* conn) {
    chownat_data_t* data = (chownat_data_t*)conn->data;
    chownat_config_t* config = (chownat_config_t*)conn->config;
    udp_session_t* session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(data == NULL) return UDP_CONN_ERR;
    if(config == NULL) return UDP_CONN_ERR;
    if(session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL) return UDP_CONN_ERR;
    if(conn->udp_conn_callback == NULL) return UDP_CONN_ERR;
    
    if(ctrl->init) return UDP_CONN_ALREADY_INIT; // already initialized

    struct timeval udp_recv_timeout = {
        .tv_sec = config->udp_recv_timeout_sec,
        .tv_usec = 0
    };

    if(setsockopt(session->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &udp_recv_timeout, sizeof(udp_recv_timeout)) < 0) {
        DEBUG_PRINT("[ERROR] Erro ao configurar setsockopt\n");
        close(session->socket_fd);
        return UDP_CONN_ERR;
    }

    if(setsockopt(session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &config->reuse, sizeof(config->reuse)) < 0) {
        DEBUG_PRINT("[ERROR] Erro ao configurar setsockopt\n");
        close(session->socket_fd);
        return UDP_CONN_ERR;
    }    

    if(bind(session->socket_fd, (struct sockaddr*)&session->src, sizeof(session->src)) < 0) {
        DEBUG_PRINT("[ERROR] bind %s\n", strerror(errno));
        return UDP_CONN_ERR;
    }  

    data->id = 0;
    data->expected = 0;
    memset(data->buffer, 0, sizeof(data->buffer));
    memset(data->sizes, 0, sizeof(data->sizes));

    ctrl->init = 1;
    data->closed = 0;

    DEBUG_PRINT("[DEBUG] chownat_init()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_deinit(const udp_conn_t* conn) {

    // depois aqui vai chamar disconnect e fazer mais alguma operação se necessário ???
    udp_session_t* udp_session = conn->udp_session;
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    chownat_data_t* data = (chownat_data_t*)conn->data;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    if(!data->closed) {
        chownat_disconnect_send(conn);
    }

    if(udp_session->socket_fd > 0)
        close(udp_session->socket_fd);

    if(tcp_session != NULL) {
        if(tcp_session->accepted_sock > 0)
            close(tcp_session->accepted_sock);
        if(tcp_session->socket_fd > 0)
            close(tcp_session->socket_fd);
    }

    ctrl->init = 0;

    DEBUG_PRINT("[DEBUG] chownat_deinit()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_is_closed(const udp_conn_t* conn) {
    chownat_data_t* data = (chownat_data_t*)conn->data;

    if(data == NULL) return UDP_CONN_ERR;

    return data->closed ? UDP_CONN_CLOSED : UDP_CONN_OK;
}

static udp_conn_status_t chownat_udp_send_ka(const udp_conn_t* conn) {
    
    udp_session_t* udp_session = conn->udp_session;
    chownat_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL) return UDP_CONN_ERR;
    
    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    int sent = sendto(udp_session->socket_fd, "\0", 1, 0, (struct sockaddr *)&udp_session->dst, sizeof(udp_session->dst));
    if(sent < 0) {
        DEBUG_PRINT("[ERROR] send %s\n", strerror(errno));
        exit(errno);
    }

    DEBUG_PRINT("[DEBUG] Sent keep-alive\n");

    return UDP_CONN_OK;
} 

static udp_conn_status_t chownat_hole_punching(const udp_conn_t* conn) {

    chownat_config_t* config = (chownat_config_t*)conn->config;
    udp_session_t* session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    int attempts = 0;

    if(config == NULL)  return UDP_CONN_ERR;
    if(session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)    return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    if(ctrl->mode == 'c') {

        DEBUG_PRINT("[DEBUG] Opening a connection to the remote end\n"); 

        while(attempts < config->conn_max_attempts) {

            DEBUG_PRINT("[DEBUG] Attempting to connect\n");

            char* msg = "01\n";

            sendto(session->socket_fd, msg, strlen(msg), 0, (struct sockaddr*)&session->dst, sizeof(session->dst));
            char buffer[4];
            
            if(recv(session->socket_fd, buffer, 3, 0) < 0) { // se socket deu erro ou timeout, tenta novamente
                attempts++;
                continue;
            }

            buffer[3] = 0;

            msg = "03\n";
            if(strncmp(buffer, msg, 3) == 0) {
                sendto(session->socket_fd, msg, strlen(msg), 0, (struct sockaddr*)&session->dst, sizeof(session->dst));
                DEBUG_PRINT("[REMOTE] Connection opened to remote end\n");
                return UDP_CONN_OK;
            }   

            attempts++; // não deve cair aqui, já vou deixar um debug pq provavelmente vai cair
            DEBUG_PRINT("[ERROR] Should not receive the message %s\n", buffer);
        }

        DEBUG_PRINT("[DEBUG] Exceded number of attempts\n");
    } else if(ctrl->mode == 's') {

        DEBUG_PRINT("[DEBUG] Waiting a connection from the remote end\n"); 
        char buffer[4];
        
        while (attempts < config->conn_max_attempts)
        {
            if(recv(session->socket_fd, buffer, 3, 0) < 0) { // se socket deu erro ou timeout, tenta novamente
                chownat_udp_send_ka(conn);
                attempts++;
                continue;
            }

            buffer[3] = 0;

            if(strncmp(buffer, "01\n", 3) == 0) {

                DEBUG_PRINT("[REMOTE] Attempted to connect to us, initializing connection\n");

                int attempts = 0; 
                
                while (attempts < config->conn_max_attempts) {
                    
                    DEBUG_PRINT("[DEBUG] Connecting...\n");
                    sendto(session->socket_fd, "03\n", 3, 0, (struct sockaddr *)&session->dst, sizeof(session->dst));
                    
                    if(recv(session->socket_fd, buffer, 3, 0) < 0) {
                        attempts++;
                        continue;
                    }

                    buffer[3] = 0;

                    if(strcmp(buffer, "03\n") == 0) {
                        DEBUG_PRINT("[REMOTE] Connection opened to remote end\n");
                        return UDP_CONN_OK; // terminou de conectar
                    } else {
                        DEBUG_PRINT("[DEBUG] Should not receive %x. Ignoring\n", buffer[1]);
                    }
                }

                DEBUG_PRINT("[DEBUG] Connection failed\n");
            }

        }
    } else {
        DEBUG_PRINT("[ERROR] mode %c not known\n", ctrl->mode);
        return UDP_CONN_ERR;
    }

    return UDP_CONN_ERR;
}

static udp_conn_status_t chownat_connect(const udp_conn_t* conn) {

    chownat_data_t *data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    
    if(data == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL) return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;
    
    conn->udp_conn_callback(conn, CHOWNAT_UDP_CONNECTED, NULL, 0);

    DEBUG_PRINT("[DEBUG] Connected!\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_disconnect_send(const udp_conn_t* conn) {

    DEBUG_PRINT("[DEBUG] Ending a connection with remote end\n"); 

    chownat_config_t* config = conn->config;
    chownat_data_t* data = conn->data;
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_session_t* udp_session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(config == NULL)      return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    char buffer[4];
    int attempts = 0;

    while(attempts < config->conn_max_attempts) {

        DEBUG_PRINT("[DEBUG] Attempting to disconnect\n");

        char* msg = "02\n";

        sendto(udp_session->socket_fd, msg, strlen(msg), 0, (struct sockaddr*)&udp_session->dst, sizeof(udp_session->dst));

        if(recv(udp_session->socket_fd, buffer, 3, 0) < 0) {
            attempts++;
            continue;
        }

        buffer[3] = 0;

        if(strcmp(buffer, "03\n") == 0) {
            sendto(udp_session->socket_fd, "03\n", strlen(msg), 0, (struct sockaddr*)&udp_session->dst, sizeof(udp_session->dst));
            break;
        }

        attempts++;
    }

    DEBUG_PRINT("[REMOTE] Connection ended with remote end\n");

    data->id = 0;
    data->expected = 0;
    
    if(tcp_session) {
        if(tcp_session->socket_fd > 0)
        {
            close(tcp_session->socket_fd);
            tcp_session->socket_fd = -1;
        }
        if(tcp_session->accepted_sock > 0)
        {
            close(tcp_session->accepted_sock);
            tcp_session->accepted_sock = -1;
        }
    }

    data->closed = 1;

    DEBUG_PRINT("[DEBUG] chownat_disconnect()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_disconnect_recv(const udp_conn_t* conn) {

    DEBUG_PRINT("[DEBUG] Attempted to disconnect us, initializing disconnection\n");

    chownat_config_t* config = conn->config;
    chownat_data_t* data = conn->data;
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_session_t* udp_session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(config == NULL)      return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;
    
    char buffer[4];
    int attempts = 0;

    while(attempts < config->conn_max_attempts) {

        DEBUG_PRINT("[DEBUG] Disconnecting...\n");

        sendto(udp_session->socket_fd, "03\n", 3, 0, (struct sockaddr*)&udp_session->dst, sizeof(udp_session->dst));

        if(recv(udp_session->socket_fd, buffer, 3, 0) < 0) {
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
    
    if(tcp_session) {
        close(tcp_session->socket_fd);
        tcp_session->socket_fd = -1;
        close(tcp_session->accepted_sock);
        tcp_session->accepted_sock = -1;
    }    

    data->closed = 1;

    DEBUG_PRINT("[DEBUG] chownat_disconnect()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_udp_send(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *sent_bytes) {

    chownat_data_t* data = (chownat_data_t*)conn->data;
    udp_session_t* udp_session = conn->udp_session;
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(tcp_session == NULL) return UDP_CONN_ERR;
    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    if(nbytes > size-3) // payload é 1021 (outros 3 são header)
        return UDP_CONN_ERR;

    char* data_in = (char *)buf;

    memcpy(&data->buffer[data->id], data_in, nbytes);
    data->sizes[data->id] = nbytes;
    
    char outbuf[size];
    outbuf[0] = '0';
    outbuf[1] = '9';
    outbuf[2] = data->id;

    data->id++;
    if(data->id == 256) data->id = 0;
    
    memcpy(&outbuf[3], data_in, nbytes);
    ssize_t sent = sendto(udp_session->socket_fd, outbuf, nbytes+3, 0, (struct sockaddr*)&udp_session->dst, sizeof(udp_session->dst));
    if(sent < 0) {
        return UDP_CONN_ERR;
    }
    if(sent_bytes != NULL) {
        *sent_bytes = sent - 3;
    }
    
    conn->udp_conn_callback(conn, CHOWNAT_UDP_DATA_SENT, data_in, nbytes);

    DEBUG_PRINT("[DEBUG] chownat_udp_send()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_udp_recv(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *recv_bytes) {

    static char msg[size];

    chownat_data_t* data = conn->data;
    udp_session_t* udp_session = conn->udp_session;
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;
    
    if(tcp_session && buf != NULL) return UDP_CONN_WITH_TCP_TUNNELING;

    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    int recvd = recv(udp_session->socket_fd, msg, size, 0);

    if(recvd < 0) {
        DEBUG_PRINT("[ERROR] recv %s\n", strerror(errno));
        data->closed = 1;
        return UDP_CONN_ERR; // used mainly when calling on callback
    }    

    else if(recvd < 3) {
        DEBUG_PRINT("[DEBUG] Received keep-alive\n"); // ignore keep-alives
    }

    else if(strncmp(msg, "02\n", 3) == 0) {
        chownat_disconnect_recv(conn);
        data->closed = 1;
        return UDP_CONN_ERR; // code for disconnect hole punching defined on udp_conn when calling udp_conn_recv (now on line 240, may be changed, but i think it wont)
    }

    else if(strncmp(msg, "03\n", 3) == 0) {
        DEBUG_PRINT("[DEBUG] handshake\n"); // mensagem extra que pode acabar vindo em caso de perda de pacote
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
            sendto(udp_session->socket_fd, outbuf, data->sizes[i], 0, (struct sockaddr*)&udp_session->dst, sizeof(udp_session->dst));
        }

        conn->udp_conn_callback(conn, CHOWNAT_UDP_LOST_DATA, msg, recvd); // return header (there is not data), but data is already retransmited
    }

    else if(strncmp(msg, "09", 2) == 0) {
        uint8_t got = msg[2];
        DEBUG_PRINT("[DEBUG] Got packet %d, expected packet %d\n", got, data->expected);

        if(got != data->expected) {
            char msg[] = "080";
            msg[2] = data->expected;
            sendto(udp_session->socket_fd, msg, sizeof(msg), 0, (struct sockaddr*)&udp_session->dst, sizeof(udp_session->dst));
        } else if(conn->tcp_session) {

            DEBUG_PRINT("[DEBUG] Received packet %d\n", got);

            if(send(tcp_session->accepted_sock, &msg[3], recvd-3, 0) < 0) {
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

    /* for cases where user wants to receive the data, from callback (it is not recomended, since it can receive a keep-alive or something) */
    int recvd_len = recvd-3;
    char *data_out = (char *)buf;
    if(data_out != NULL) {
        if(recvd_len > nbytes) {
            memcpy(data_out, &msg[3], nbytes);
            *recv_bytes = nbytes; // -3 is because of "header", passing just DATA
            return UDP_CONN_OK_TRUNCATED; // truncated, because buffer is smaller than received data
        } else {
            memcpy(data_out, &msg[3], recvd_len);
            *recv_bytes = recvd_len; // -3 is because of "header", passing just DATA
        }
    }

    return UDP_CONN_OK;
}

udp_conn_generic_api_t chownat_api = {
    .init = chownat_init,
    .deinit = chownat_deinit,
    .is_closed = chownat_is_closed,
    .hole_punching = chownat_hole_punching,
    .connect = chownat_connect,
    .udp_send = chownat_udp_send,
    .udp_recv = chownat_udp_recv,
    .udp_send_ka = chownat_udp_send_ka,
    .disconnect = chownat_disconnect_send,
};