#include "udp_conn.h"
#include <stdint.h>

static int monotonic_us(int64_t *value) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return -1;

    *value = (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    return 0;
}

static udp_conn_status_t tcp_bind(const udp_conn_t *conn) {

    tcp_session_t* tcp_session = conn->tcp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(tcp_session == NULL) return UDP_CONN_WITHOUT_TCP_TUNNELING;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    if(ctrl->mode == 'c') {
        DEBUG_PRINT("[DEBUG] Binding a new socket to %d\n", ntohs(tcp_session->local.sin_port));

        if(setsockopt(tcp_session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &tcp_session->reuse, sizeof(tcp_session->reuse)) < 0) {
            DEBUG_PRINT("Erro ao configurar setsockopt\n");
            close(tcp_session->socket_fd);
            tcp_session->socket_fd = -1;
            return UDP_CONN_ERR;
        }    

        if(bind(tcp_session->socket_fd, (struct sockaddr *)&tcp_session->local, sizeof(tcp_session->local)) < 0) {
            DEBUG_PRINT("Erro ao fazer o bind: %s\n", strerror(errno));
            close(tcp_session->socket_fd);
            tcp_session->socket_fd = -1;
            return UDP_CONN_ERR;
        }

        if(listen(tcp_session->socket_fd, 20) < 0) {
            DEBUG_PRINT("Erro ao escutar\n");
            close(tcp_session->socket_fd);
            tcp_session->socket_fd = -1;
            return UDP_CONN_ERR;
        }

        DEBUG_PRINT("[DEBUG] Esperando conexão...\n");

        tcp_session->accepted_sock = accept(tcp_session->socket_fd, 0, 0);

        if(tcp_session->accepted_sock < 0) 
            return UDP_CONN_ERR;

        conn->transport->tcp_fd = tcp_session->accepted_sock;

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
        conn->transport->tcp_fd = tcp_session->accepted_sock;

        DEBUG_PRINT("[DEBUG] connection to local daemon (port %d) opened\n", tcp_session->local.sin_port);

    } else {
        DEBUG_PRINT("[ERROR] mode %c not known\n", ctrl->mode);
        return UDP_CONN_ERR;
    }

    return UDP_CONN_OK;
}

static udp_conn_status_t tcp_recv(const udp_conn_t* conn) {

    static char msg[TCP_MTU_SIZE];
    tcp_session_t* tcp_session = conn->tcp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

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
        return conn->api->udp_send(conn, msg, recvd, NULL);
    }
}

/* Shared delivery for socket readiness and backend-buffered payloads. */
static udp_conn_status_t tcp_send(const udp_conn_t *conn, char *buf, size_t nbytes) {
    if (conn->tcp_session != NULL) {
        size_t offset = 0;
        
        while (offset < nbytes) {
            ssize_t sent = send(conn->tcp_session->accepted_sock,
                                buf + offset, nbytes - offset, MSG_NOSIGNAL);
            if (sent < 0 && errno == EINTR) continue;
            if (sent <= 0) return UDP_CONN_ERR;
            offset += (size_t)sent;
        }
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
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    char buf[TCP_MTU_SIZE];

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;
    if(conn->transport == NULL || !conn->transport->opened) return UDP_CONN_NOT_INIT;
    
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
    const int64_t ka_interval_us = 5LL * 1000000;
    int64_t now_us;

    if (monotonic_us(&now_us) < 0) return UDP_CONN_ERR;
    int64_t next_ka_us = now_us + ka_interval_us;

    while (!conn->api->is_closed(conn))
    {   
        if (conn->api->process_events != NULL) {
            udp_conn_status_t status = conn->api->process_events(conn);

            if (status < 0) return status;
            if (conn->api->is_closed(conn)) break;
        }

        while (conn->api->pending_data != NULL &&
               conn->api->pending_data(conn) > 0) {
            size_t received = 0;
            udp_conn_status_t status;

            status = conn->api->udp_recv(conn, buf, sizeof(buf), &received);

            if (status < 0) {
                if (conn->api->is_closed(conn)) break;
                conn->api->disconnect(conn);
                return status;
            }

            if (conn->api->is_closed(conn)) break;

            status = tcp_send(conn, buf, received);

            if (status < 0) {
                conn->api->disconnect(conn);
                return status;
            }

            if (conn->api->is_closed(conn) || received == 0) break;

            if (monotonic_us(&now_us) < 0) return UDP_CONN_ERR;
            next_ka_us = now_us + ka_interval_us;
            threshold = 0;
        }

        if (conn->api->is_closed(conn)) break;

        unsigned events = 0;
        struct timeval wait_timeout = {0};

        udp_conn_status_t tstat = conn->api->get_timeout(conn, &wait_timeout);
        if (tstat != UDP_CONN_OK && tstat != UDP_CONN_NOT_APPLICABLE) {
            DEBUG_PRINT("[ERROR] Error while getting timeout\n");
            return tstat;
        }

        if (monotonic_us(&now_us) < 0) return UDP_CONN_ERR;
        int64_t remaining_us = next_ka_us > now_us ? next_ka_us - now_us : 0;

        /* Wake for the backend's next event or the keep-alive deadline. */
        if (tstat == UDP_CONN_NOT_APPLICABLE ||
            remaining_us < (int64_t)wait_timeout.tv_sec * 1000000 + wait_timeout.tv_usec) {
            wait_timeout.tv_sec = remaining_us / 1000000;
            wait_timeout.tv_usec = remaining_us % 1000000;
        }

        if(udp_transport_wait(conn->transport, &wait_timeout, &events) != UDP_TRANSPORT_OK)
            return UDP_CONN_ERR;

        if(events != 0) {
            threshold = 0;
            
            if(events & TCP_TRANSPORT_READABLE) {
                DEBUG_PRINT("[DEBUG] Data received on socket TCP\n");
                if(tcp_recv(conn) < 0) {
                    conn->api->disconnect(conn);
                }
            }
            if (conn->api->is_closed(conn)) break;

            if(events & UDP_TRANSPORT_READABLE) {
                DEBUG_PRINT("[DEBUG] Data received on socket UDP\n");
                size_t received = 0;
                udp_conn_status_t status = conn->api->udp_recv(conn, buf, sizeof(buf), &received);

                if (status < 0) {
                    if (conn->api->is_closed(conn)) break;
                    conn->api->disconnect(conn);
                    return status;
                }

                if (conn->api->is_closed(conn)) break;

                status = tcp_send(conn, buf, received);
                if (status < 0) {
                    conn->api->disconnect(conn);
                    return status;
                }
            }

            /* Socket I/O restarts the inactivity interval. */
            if (monotonic_us(&now_us) < 0) return UDP_CONN_ERR;
            next_ka_us = now_us + ka_interval_us;
        } else { // events == 0
            if (conn->api->is_closed(conn)) break;
            if (monotonic_us(&now_us) < 0) return UDP_CONN_ERR;

            /* A backend event timeout alone does not trigger keep-alive. */
            if (now_us >= next_ka_us) {

                udp_conn_status_t status = conn->api->udp_send_ka(conn);
                if (status != UDP_CONN_OK) 
                    return status;

                if (conn->api->is_closed(conn)) 
                    break;

                if (monotonic_us(&now_us) < 0) 
                    return UDP_CONN_ERR;
                    
                next_ka_us = now_us + ka_interval_us;

                // Backend event timers do not count as missed keep-alives.
                if (conn->api->process_events == NULL) {
                    if (threshold == udp_session->ka_miss_threshold)
                        conn->api->disconnect(conn);
                    threshold++;
                }
            }
        }
    }    

    return UDP_CONN_OK;
}
