#include "udp_conn.h"

int initiated = 0;
int closed = 0;
            
fd_set read_fds;
int ready = 0;
int sock = -1;

int udp_conn_init(struct udp_conn_t *conn) {

    int ret = 0;
    if(conn->api) {
        ret = conn->api->init(conn);
        closed = 0;
        initiated = 1;
        return ret;
    }
    else 
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

int udp_conn_deinit(struct udp_conn_t* conn) {

    int ret = 0;
    if(conn->api) {
        ret = conn->api->deinit(conn);
        initiated = 0;
        closed = 0;
        return ret;
    }
    else 
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

static int udp_conn_hole_punching(const struct udp_conn_t *conn) {
    
    if(conn->api)
        return conn->api->hole_punching(conn);
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

static int udp_conn_connect(const struct udp_conn_t *conn) {

    if(conn->api)
        return conn->api->connect(conn);
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

size_t udp_conn_send(const struct udp_conn_t *conn, void *data, size_t nbytes) {
        
    if(conn->api)
        return conn->api->udp_send(conn, data, nbytes);
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

size_t udp_conn_recv(const struct udp_conn_t *conn) { 
        
    if(conn->api)
        return conn->api->udp_recv(conn);
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

static int udp_conn_send_ka(const struct udp_conn_t* conn) {
    if(conn->api)
        return conn->api->udp_send_ka(conn);
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

int udp_conn_disconnect(const struct udp_conn_t *conn) {

    int ret = 0;
    if(conn->api) {
        ret = conn->api->disconnect(conn);
        closed = 1;
        return ret;
    }
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

static int tcp_recv(const struct udp_conn_t* conn) {
    if(conn->api) {
        if(conn->tcp_tun)
            return conn->api->tcp_recv(conn);
        else
            DEBUG_PRINT("[CRITICAL] TCP tun not enabled (shouldn't be here)\n");

        exit(0);
    }
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

static int tcp_bind(const struct udp_conn_t* conn) {
    if(conn->api) {
        if(conn->tcp_tun)
            return conn->api->tcp_bind(conn);
        else
            DEBUG_PRINT("[CRITICAL] TCP tun not enabled (shouldn't be here)\n");

        exit(0);
    }
    else
        DEBUG_PRINT("[ERROR] Api not implemented\n");

    return -1;
}

/*
    This function bellow that doesn't simply call the conn api is the user access function, 
    it is the base flow of chownat idea for holepunching that is based in some states 
    (keep in a full loop using keep alives)
    more states can be implemented, this is just a basic idea
*/
int udp_connection(const struct udp_conn_t *conn) {

    // aqui começaria com init, mas já fizemos antes de entrar aqui, então

    if(!initiated) return -1;

    // separação entre cliente e server
    
    if(conn->session->mode == 'c') {

        // diferente do original, após finalizar uma conexão (disconnect), não fica esperando em loop por uma nova tentativa com conexão TCP
        if(conn->tcp_tun) {
            if(tcp_bind(conn) < 0) // pode, ou não, fazer tunneling de TCP
                return -1;

            conn->tcp_tun->accepted_sock = accept(conn->tcp_tun->socket_fd, 0, 0);
    
            if(conn->tcp_tun->accepted_sock < 0) return -1;

            sock = conn->tcp_tun->accepted_sock;
        }

        if(udp_conn_hole_punching(conn) < 0) return -1; // aqui inicia o hole_punching
        if(udp_conn_connect(conn) < 0) return -1; // um passo extra para casos onde existe o uso de um protocolo adicional (eg. QUIC)

        while (!closed)
        {
            struct timeval ka_timeout = {
                .tv_sec = 5,
                .tv_usec = 0
            };

            FD_ZERO(&read_fds);
            FD_SET(conn->session->socket_fd, &read_fds);
            if(conn->tcp_tun)
                FD_SET(conn->tcp_tun->accepted_sock, &read_fds);

            ready = select(max(conn->session->socket_fd, sock)+1, &read_fds, NULL, NULL, &ka_timeout);

            if(ready < 0) {
                DEBUG_PRINT("[ERROR] select error %s\n", strerror(errno));
                exit(errno);
            } else if(ready == 0) {
                // timeout: send keep alive
                udp_conn_send_ka(conn);
            } else {
                DEBUG_PRINT("[DEBUG] some message has been received at %d\n", ready);

                if(sock != -1 && FD_ISSET(sock, &read_fds)) {
                    if(tcp_recv(conn) < 0)
                        conn->api->disconnect(conn);
                }
                if(FD_ISSET(conn->session->socket_fd, &read_fds)) {
                    if(!udp_conn_recv(conn))
                        closed = 1; // kinda disconnect (or an error)
                }
            }
        }

    } else if(conn->session->mode == 's') {

        if(udp_conn_hole_punching(conn) < 0) return -1;

        if(conn->tcp_tun) {
            if(tcp_bind(conn) < 0) // pode, ou não, fazer tunneling de TCP
                return -1;

            if(conn->tcp_tun->accepted_sock < 0) return -1;

            sock = conn->tcp_tun->accepted_sock;
        }

        if(udp_conn_connect(conn) < 0) return -1;

        int threshold = 0;

        while (!closed)
        {   
            struct timeval ka_timeout = {
                .tv_sec = 5,
                .tv_usec = 0
            };

            FD_ZERO(&read_fds);
            FD_SET(conn->session->socket_fd, &read_fds);
            if(conn->tcp_tun)
                FD_SET(conn->tcp_tun->accepted_sock, &read_fds);

            ready = select(max(conn->session->socket_fd, sock)+1, &read_fds, NULL, NULL, &ka_timeout);

            if(ready < 0) {
                DEBUG_PRINT("[ERROR] select %s\n", strerror(errno));
                exit(errno);
            } else if(ready == 0) {
                // timeout: send keep alive
                udp_conn_send_ka(conn);

                if(threshold == conn->session->ka_miss_threshold)
                    udp_conn_disconnect(conn);

                threshold++;
            } else {
                threshold = 0;
                // DEBUG_PRINT("[DEBUG] some message has been received at %d\n", ready);

                if(sock != -1 && FD_ISSET(sock, &read_fds)) {
                    tcp_recv(conn);
                }
                if(FD_ISSET(conn->session->socket_fd, &read_fds)) {
                    if(!udp_conn_recv(conn))
                        closed = 1; // kinda disconnect (or an error)
                }
            }
        }    
    } else {    
        DEBUG_PRINT("[ERROR] Unknown mode %c\n", conn->session->mode);
        return -1;
    }

    return 0;
}