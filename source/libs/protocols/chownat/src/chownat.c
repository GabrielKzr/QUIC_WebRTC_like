#include "chownat.h"
#include "utils.h"

static udp_conn_status_t chownat_init(const udp_conn_t* conn);
static udp_conn_status_t chownat_deinit(const udp_conn_t* conn);
static udp_conn_status_t chownat_is_closed(const udp_conn_t* conn);
static udp_conn_status_t chownat_get_timeout(const udp_conn_t* conn, struct timeval *tv);
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
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(data == NULL) return UDP_CONN_ERR;
    if(config == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL) return UDP_CONN_ERR;
    if(conn->udp_conn_callback == NULL) return UDP_CONN_ERR;
    if(conn->transport == NULL || !conn->transport->opened) return UDP_CONN_NOT_INIT;
    
    if(ctrl->init) return UDP_CONN_ALREADY_INIT; // already initialized

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
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    chownat_data_t* data = (chownat_data_t*)conn->data;

    if(ctrl == NULL)        return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    if(!data->closed) {
        chownat_disconnect_send(conn);
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

static udp_conn_status_t chownat_get_timeout(const udp_conn_t* conn, struct timeval *tv) {

    udp_conn_ctrl_t *ctrl = conn->ctrl;

    if(ctrl == NULL) return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    tv->tv_sec = 5;    
    tv->tv_usec = 0;
    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_udp_send_ka(const udp_conn_t* conn) {
    
    chownat_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(data == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL) return UDP_CONN_ERR;
    
    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    struct timeval timeout = {0};
    if(udp_transport_send(conn->transport, "\0", 1, &timeout) != UDP_TRANSPORT_OK) {
        return UDP_CONN_ERR;
    }

    DEBUG_PRINT("[DEBUG] Sent keep-alive\n");

    return UDP_CONN_OK;
} 

static udp_conn_status_t chownat_hole_punching(const udp_conn_t* conn) {

    chownat_config_t* config = (chownat_config_t*)conn->config;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    int attempts = 0;

    if(config == NULL)  return UDP_CONN_ERR;
    if(ctrl == NULL)    return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    struct timeval send_timeout = {0};
    struct timeval recv_timeout = {.tv_sec = config->udp_recv_timeout_sec};
    size_t received = 0;

    if(ctrl->mode == 'c') {

        DEBUG_PRINT("[DEBUG] Opening a connection to the remote end\n"); 

        while(attempts < config->conn_max_attempts) {

            DEBUG_PRINT("[DEBUG] Attempting to connect\n");

            char* msg = "01\n";

            if(udp_transport_send(conn->transport, msg, strlen(msg), &send_timeout) != UDP_TRANSPORT_OK)
                return UDP_CONN_ERR;
            char buffer[4];
            
            if(udp_transport_recv(conn->transport, buffer, 3, &received, &recv_timeout) != UDP_TRANSPORT_OK) { // se socket deu erro ou timeout, tenta novamente
                attempts++;
                continue;
            }

            buffer[3] = 0;

            msg = "03\n";
            if(strncmp(buffer, msg, 3) == 0) {
                if(udp_transport_send(conn->transport, msg, strlen(msg), &send_timeout) != UDP_TRANSPORT_OK)
                    return UDP_CONN_ERR;
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
            if(udp_transport_recv(conn->transport, buffer, 3, &received, &recv_timeout) != UDP_TRANSPORT_OK) { // se socket deu erro ou timeout, tenta novamente
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
                    if(udp_transport_send(conn->transport, "03\n", 3, &send_timeout) != UDP_TRANSPORT_OK)
                        return UDP_CONN_ERR;
                    
                    if(udp_transport_recv(conn->transport, buffer, 3, &received, &recv_timeout) != UDP_TRANSPORT_OK) {
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
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(config == NULL)      return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    struct timeval send_timeout = {0};
    struct timeval recv_timeout = {.tv_sec = config->udp_recv_timeout_sec};
    size_t received = 0;

    char buffer[4];
    int attempts = 0;

    while(attempts < config->conn_max_attempts) {

        DEBUG_PRINT("[DEBUG] Attempting to disconnect\n");

        char* msg = "02\n";

        if(udp_transport_send(conn->transport, msg, strlen(msg), &send_timeout) != UDP_TRANSPORT_OK)
            return UDP_CONN_ERR;

        if(udp_transport_recv(conn->transport, buffer, 3, &received, &recv_timeout) != UDP_TRANSPORT_OK) {
            attempts++;
            continue;
        }

        buffer[3] = 0;

        if(strcmp(buffer, "03\n") == 0) {
            if(udp_transport_send(conn->transport, "03\n", strlen(msg), &send_timeout) != UDP_TRANSPORT_OK)
                return UDP_CONN_ERR;
            break;
        }

        attempts++;
    }

    DEBUG_PRINT("[REMOTE] Connection ended with remote end\n");

    data->id = 0;
    data->expected = 0;
    

    data->closed = 1;

    DEBUG_PRINT("[DEBUG] chownat_disconnect()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_disconnect_recv(const udp_conn_t* conn) {

    DEBUG_PRINT("[DEBUG] Attempted to disconnect us, initializing disconnection\n");

    chownat_config_t* config = conn->config;
    chownat_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(config == NULL)      return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    struct timeval send_timeout = {0};
    struct timeval recv_timeout = {.tv_sec = config->udp_recv_timeout_sec};
    size_t received = 0;
    
    char buffer[4];
    int attempts = 0;

    while(attempts < config->conn_max_attempts) {

        DEBUG_PRINT("[DEBUG] Disconnecting...\n");

        if(udp_transport_send(conn->transport, "03\n", 3, &send_timeout) != UDP_TRANSPORT_OK)
            return UDP_CONN_ERR;

        if(udp_transport_recv(conn->transport, buffer, 3, &received, &recv_timeout) != UDP_TRANSPORT_OK) {
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
    

    data->closed = 1;

    DEBUG_PRINT("[DEBUG] chownat_disconnect()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_udp_send(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *sent_bytes) {

    chownat_data_t* data = (chownat_data_t*)conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

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
    struct timeval timeout = {0};
    if(udp_transport_send(conn->transport, outbuf, nbytes+3, &timeout) != UDP_TRANSPORT_OK) {
        return UDP_CONN_ERR;
    }
    if(sent_bytes != NULL) {
        *sent_bytes = nbytes;
    }
    
    conn->udp_conn_callback(conn, CHOWNAT_UDP_DATA_SENT, data_in, nbytes);

    DEBUG_PRINT("[DEBUG] chownat_udp_send()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t chownat_udp_recv(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *recv_bytes) {

    static char msg[size];

    chownat_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;
    
    if (recv_bytes == NULL || buf == NULL) return UDP_CONN_ERR;
    *recv_bytes = 0;

    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;

    size_t recvd = 0;
    struct timeval timeout = {0};

    if(udp_transport_recv(conn->transport, msg, sizeof(msg), &recvd, &timeout) != UDP_TRANSPORT_OK) {
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
            if(udp_transport_send(conn->transport, outbuf, data->sizes[i], &timeout) != UDP_TRANSPORT_OK)
                return UDP_CONN_ERR;
        }

        conn->udp_conn_callback(conn, CHOWNAT_UDP_LOST_DATA, msg, recvd); // return header (there is not data), but data is already retransmited
    }

    else if(strncmp(msg, "09", 2) == 0) {
        uint8_t got = msg[2];
        DEBUG_PRINT("[DEBUG] Got packet %d, expected packet %d\n", got, data->expected);

        if(got != data->expected) {
            char msg[] = "080";
            msg[2] = data->expected;
            if(udp_transport_send(conn->transport, msg, sizeof(msg), &timeout) != UDP_TRANSPORT_OK)
                return UDP_CONN_ERR;
        } else {
            data->expected++;
            if(data->expected == 256) data->expected = 0;
            size_t payload = (size_t)recvd - 3;
            size_t copied = payload > nbytes ? nbytes : payload;
            memcpy(buf, msg + 3, copied);
            *recv_bytes = copied;
            conn->udp_conn_callback(conn, CHOWNAT_UDP_RECV_DATA, buf, copied);
            if (copied < payload) return UDP_CONN_OK_TRUNCATED;
        }
    }

    return UDP_CONN_OK;
}

udp_conn_generic_api_t chownat_api = {
    .init = chownat_init,
    .deinit = chownat_deinit,
    .is_closed = chownat_is_closed,
    .get_timeout = chownat_get_timeout,
    .hole_punching = chownat_hole_punching,
    .connect = chownat_connect,
    .udp_send = chownat_udp_send,
    .udp_recv = chownat_udp_recv,
    .udp_send_ka = chownat_udp_send_ka,
    .disconnect = chownat_disconnect_send,
};
