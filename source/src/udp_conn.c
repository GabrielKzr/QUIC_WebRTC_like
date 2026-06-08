#include "udp_conn.h"

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
    if(tcp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;
    
    if(!ctrl->init) return UDP_CONN_NOT_INIT;
    
    // separação entre cliente e server
    if(ctrl->mode == 'c') {

        // diferente do original, após finalizar uma conexão (disconnect), não fica esperando em loop por uma nova tentativa com conexão TCP
        if(conn->api->tcp_bind(conn) < 0)      return UDP_CONN_TCP_BIND_ERROR; // se tcp_bind retornar -1, é porque não tem tcp_session, então não tem tunneling de TCP, então só continua o fluxo normalmente
        if(conn->api->hole_punching(conn) < 0) return UDP_CONN_TCP_HP_ERROR; // aqui inicia o hole_punching
        if(conn->api->connect(conn) < 0)       return UDP_CONN_CONNECT_ERROR; // um passo extra principalmente usado para casos onde existe o uso de um protocolo adicional (eg. QUIC)

        while (!conn->api->is_closed(conn))
        {
            // preciso fazer o handle disso aqui tudo aqui em baixo traduzindo pra api nova, (VAI DAR TRABALHO, MAS VAI FICAR MELHOR DEPOIS, ACHO QUE VAI FICAR MAIS LIMPO DEPOIS)
            int ready = 0;
            int tcp_fd = tcp_session ? tcp_session->accepted_sock : -1;
            int udp_fd = udp_session ? udp_session->socket_fd : -1;
            struct timeval ka_timeout = {
                .tv_sec = 5,
                .tv_usec = 0
            };

            FD_ZERO(&ctrl->read_fds);
            FD_SET(udp_fd, &ctrl->read_fds);
            FD_SET(tcp_fd, &ctrl->read_fds);

            ready = select(max(udp_fd, tcp_fd)+1, &ctrl->read_fds, NULL, NULL, &ka_timeout);

            if(ready < 0) {
                DEBUG_PRINT("[ERROR] select error %s\n", strerror(errno));
                exit(errno);
            } else if(ready == 0) {
                // timeout: send keep alive
                conn->api->udp_send_ka(conn);
            } else {
                DEBUG_PRINT("[DEBUG] some message has been received at %d\n", ready);

                if(tcp_fd != -1 && FD_ISSET(tcp_fd, &ctrl->read_fds)) {
                    if(conn->api->tcp_recv(conn) < 0) {
                        conn->api->disconnect(conn);
                    }
                }
                if(udp_fd != -1 && FD_ISSET(udp_fd, &ctrl->read_fds)) {
                    if(conn->api->udp_recv(conn) < 0 && !conn->api->is_closed(conn)) { 
                        conn->api->disconnect(conn);
                    }
                }
            }
        }

    } else if(ctrl->mode == 's') {

        if(conn->api->hole_punching(conn) < 0) return UDP_CONN_TCP_HP_ERROR;
        if(conn->api->tcp_bind(conn) < 0)      return UDP_CONN_TCP_BIND_ERROR; // se tcp_bind retornar -1, é porque não tem tcp_session, então não tem tunneling de TCP, então só continua o fluxo normalmente
        if(conn->api->connect(conn) < 0)       return UDP_CONN_CONNECT_ERROR;

        int threshold = 0;

        while (!conn->api->is_closed(conn))
        {   
            int ready = 0;
            int tcp_fd = tcp_session ? tcp_session->accepted_sock : -1;
            int udp_fd = udp_session ? udp_session->socket_fd : -1;
            struct timeval ka_timeout = {
                .tv_sec = 5,
                .tv_usec = 0
            };

            FD_ZERO(&ctrl->read_fds);
            FD_SET(udp_fd, &ctrl->read_fds);
            FD_SET(tcp_fd, &ctrl->read_fds);

            ready = select(max(udp_fd, tcp_fd)+1, &ctrl->read_fds, NULL, NULL, &ka_timeout);

            if(ready < 0) {
                DEBUG_PRINT("[ERROR] select %s\n", strerror(errno));
                exit(errno);
            } else if(ready == 0) {
                // timeout: send keep alive
                conn->api->udp_send_ka(conn);

                if(threshold == udp_session->ka_miss_threshold)
                    conn->api->disconnect(conn);

                threshold++;
            } else {
                threshold = 0;

                if(tcp_fd != -1 && FD_ISSET(tcp_fd, &ctrl->read_fds)) {
                    if(conn->api->tcp_recv(conn) < 0) {
                        conn->api->disconnect(conn);
                    }
                }
                if(udp_fd != -1 && FD_ISSET(udp_fd, &ctrl->read_fds)) {
                    if(conn->api->udp_recv(conn) < 0 && !conn->api->is_closed(conn)) {
                        conn->api->disconnect(conn);
                    }
                }
            }
        }    
    } else {    
        DEBUG_PRINT("[ERROR] Unknown mode %c\n", ctrl->mode);
        return UDP_CONN_UNKNOWN_MODE;
    }

    return UDP_CONN_OK;
}