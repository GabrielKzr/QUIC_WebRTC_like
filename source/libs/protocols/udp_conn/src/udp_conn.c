#include "udp_conn.h"

static udp_conn_status_t tcp_bind(const udp_conn_t *conn) {

    tcp_session_t* tcp_session = conn->tcp_session;
    udp_session_t* udp_session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(tcp_session == NULL) return UDP_CONN_WITHOUT_TCP_TUNNELING;
    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    if(ctrl->mode == 'c') {
        DEBUG_PRINT("[DEBUG] Binding a new socket to %d\n", ntohs(tcp_session->local.sin_port));

        if(setsockopt(tcp_session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &tcp_session->reuse, sizeof(tcp_session->reuse)) < 0) {
            DEBUG_PRINT("Erro ao configurar setsockopt\n");
            close(tcp_session->socket_fd);
            return UDP_CONN_ERR;
        }    

        if(bind(tcp_session->socket_fd, (struct sockaddr *)&tcp_session->local, sizeof(tcp_session->local)) < 0) {
            DEBUG_PRINT("Erro ao fazer o bind: %s\n", strerror(errno));
            close(tcp_session->socket_fd);
            return UDP_CONN_ERR;
        }

        if(listen(tcp_session->socket_fd, 20) < 0) {
            DEBUG_PRINT("Erro ao escutar\n");
            close(tcp_session->socket_fd);
            return UDP_CONN_ERR;
        }

        DEBUG_PRINT("[DEBUG] Esperando conexão...\n");

        tcp_session->accepted_sock = accept(tcp_session->socket_fd, 0, 0);

        if(tcp_session->accepted_sock < 0) 
            return UDP_CONN_ERR;

        return UDP_CONN_OK;

    } else if(ctrl->mode == 's') {

        if(tcp_session->socket_fd < 0) {
            DEBUG_PRINT("[ERROR] socket %s\n", strerror(errno));
            exit(errno);
        }

        if(connect(tcp_session->socket_fd, (struct sockaddr*)&tcp_session->local, sizeof(tcp_session->local)) < 0) {
            DEBUG_PRINT("[ERROR] connect %s\n", strerror(errno));
            exit(errno);
        }

        tcp_session->accepted_sock = tcp_session->socket_fd; // equal file descriptor, server does not have client to be accepted

        DEBUG_PRINT("[DEBUG] connection to local daemon (port %d) opened\n", tcp_session->local.sin_port);

    } else {
        DEBUG_PRINT("[ERROR] mode %c not known\n", ctrl->mode);
        return UDP_CONN_ERR;
    }

    return UDP_CONN_OK;
}

static udp_conn_status_t tcp_recv(const udp_conn_t* conn) {

    static char msg[TCP_MTU_SIZE];
    udp_session_t* udp_session = conn->udp_session;
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(tcp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;
    
    if(!ctrl->init)  return UDP_CONN_NOT_INIT;
    if(conn->api->is_closed(conn)) return UDP_CONN_CLOSED;

    int recvd = recv(tcp_session->accepted_sock, msg, sizeof(msg), 0);  

    DEBUG_PRINT("[DEBUG] Received %d bytes from TCP tunnel\n", recvd);

    if(recvd == 0) {
        DEBUG_PRINT("[REMOTE] Attempted to disconnect us\n");
        return UDP_CONN_ERR;
    } else if(recvd < 0) {
        DEBUG_PRINT("[ERROR] recv %s\n", strerror(errno));
        return UDP_CONN_ERR;
    } else {
        conn->api->udp_send(conn, msg, recvd, NULL);
    }

    return UDP_CONN_OK;
}

/*
    This function bellow that doesn't simply call the conn api is the user access function, 
    it is the base flow of chownat idea for holepunching that is based in some states 
    (keep in a full loop using keep alives)
    more states can be implemented, this is just a basic idea
*/
udp_conn_status_t udp_connection(const udp_conn_t *conn) {

    udp_session_t* udp_session = conn->udp_session;
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;
    
    if(!ctrl->init) return UDP_CONN_NOT_INIT;
    
    // separação entre cliente e server
    if(ctrl->mode == 'c') {

        // diferente do original, após finalizar uma conexão (disconnect), não fica esperando em loop por uma nova tentativa com conexão TCP
        if(tcp_bind(conn) < 0)                 return UDP_CONN_TCP_BIND_ERROR;
        if(conn->api->hole_punching(conn) < 0) return UDP_CONN_TCP_HP_ERROR; 
        if(conn->api->connect(conn) < 0)       return UDP_CONN_CONNECT_ERROR; 

    } else if(ctrl->mode == 's') {

        if(conn->api->hole_punching(conn) < 0) return UDP_CONN_TCP_HP_ERROR;
        if(tcp_bind(conn) < 0)                 return UDP_CONN_TCP_BIND_ERROR;
        if(conn->api->connect(conn) < 0)       return UDP_CONN_CONNECT_ERROR;

    } else {
        DEBUG_PRINT("[ERROR] Unknown mode %c\n", ctrl->mode);
        return UDP_CONN_UNKNOWN_MODE;
    }

    int threshold = 0;

    while (!conn->api->is_closed(conn))
    {   
        int ready = 0;
        int tcp_fd = tcp_session ? tcp_session->accepted_sock : -1;
        int udp_fd = udp_session ? udp_session->socket_fd : -1;
        struct timeval ka_timeout;

        udp_conn_status_t tstat = conn->api->get_timeout(conn, &ka_timeout);
        DEBUG_PRINT("[DEBUG] get_timeout status=%d sec=%ld usec=%ld\n", tstat, (long)ka_timeout.tv_sec, (long)ka_timeout.tv_usec);
        if (tstat == UDP_CONN_OK) {
            /* Do nothing */
        }
        else if (tstat == UDP_CONN_NOT_APPLICABLE) {
            ka_timeout.tv_sec = 5;
            ka_timeout.tv_usec = 0;
        } else {
            DEBUG_PRINT("[ERROR] Error while getting timeout\n");
            return UDP_CONN_ERR; // só aborta em erro real
        }

        FD_ZERO(&ctrl->read_fds);

        if(udp_fd != -1)
            FD_SET(udp_fd, &ctrl->read_fds);
        else
            DEBUG_PRINT("[ERROR] UDP socket is not initialized\n");
        if(tcp_fd != -1)
            FD_SET(tcp_fd, &ctrl->read_fds);
        else
            DEBUG_PRINT("[DEBUG] TCP socket is not initialized, skipping\n");

        ready = select(max(udp_fd, tcp_fd)+1, &ctrl->read_fds, NULL, NULL, &ka_timeout);

        DEBUG_PRINT("[DEBUG] select() returned %d\n", ready);
        
        if(ready < 0) { 
            DEBUG_PRINT("[ERROR] select %s\n", strerror(errno));
            exit(errno);
        } else if(ready == 0) {
            // timeout: send keep alive
            conn->api->udp_send_ka(conn);

            DEBUG_PRINT("[DEBUG] Keep-alive sent, threshold: %d\n", threshold);

            if(threshold == udp_session->ka_miss_threshold)
                conn->api->disconnect(conn);

            threshold++;
        } else {
            threshold = 0;
            
            if(tcp_fd != -1 && FD_ISSET(tcp_fd, &ctrl->read_fds)) {
                DEBUG_PRINT("[DEBUG] Data received on socket TCP\n");
                if(tcp_recv(conn) < 0) {
                    conn->api->disconnect(conn);
                }
            }
            if(udp_fd != -1 && FD_ISSET(udp_fd, &ctrl->read_fds)) {
                DEBUG_PRINT("[DEBUG] Data received on socket UDP\n");
                if(conn->api->udp_recv(conn, NULL, 0, NULL) < 0 && !conn->api->is_closed(conn)) {
                    conn->api->disconnect(conn);
                }
            }
        }
    }    

    return UDP_CONN_OK;
}