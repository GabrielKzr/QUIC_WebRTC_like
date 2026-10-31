#ifndef UDP_CONN_H
#define UDP_CONN_H

#include <sys/select.h>
#include <unistd.h>
#include "utils.h"
#include "udp_conn_stream.h"
#include "udp_transport.h"

#define localhost "127.0.0.1"

#ifndef TCP_MTU_SIZE
#define TCP_MTU_SIZE 1024
#endif

typedef struct sockaddr_in sockaddr_in;
typedef enum {
    UDP_CONN_NOT_APPLICABLE = 4,
    UDP_CONN_WITH_TCP_TUNNELING = 3,
    UDP_CONN_WITHOUT_TCP_TUNNELING = 2,
    UDP_CONN_NOT_CLOSED = 1,
    UDP_CONN_OK = 0,
    UDP_CONN_ERR = -1,
    UDP_CONN_ALREADY_INIT = -2,
    UDP_CONN_NOT_INIT = -3,
    UDP_CONN_TCP_BIND_ERROR = -4,
    UDP_CONN_TCP_HP_ERROR = -5,
    UDP_CONN_CONNECT_ERROR = -6,
    UDP_CONN_UNKNOWN_MODE = -7,
    UDP_CONN_CLOSED = -8,
    UDP_CONN_OK_TRUNCATED = -9,
} udp_conn_status_t;

typedef struct {
    int socket_fd;
    int ka_miss_threshold;
    sockaddr_in dst;
    sockaddr_in src;  
} udp_session_t;

typedef struct {
    int socket_fd;
    int accepted_sock;
    int reuse; 
    sockaddr_in local;
} tcp_session_t;

typedef struct {
    char mode;
    int init;
} udp_conn_ctrl_t;

// baseado em callback, então as funções abaixo serão executadas quando 
// o protocolo for executar uma dessas operações
// essas funções são definidas previamente e chamadas quando uma dessas
// operações for realizadas
// A operação task é a operação responsável por gerenciar estados após conexão
typedef struct {
    udp_conn_status_t (*init)(const udp_conn_t*);
    udp_conn_status_t (*deinit)(const udp_conn_t*);
    udp_conn_status_t (*is_closed)(const udp_conn_t*);
    udp_conn_status_t (*get_timeout)(const udp_conn_t* conn, struct timeval *tv);
    // Backend opcional: processa timers e dados internos antes de esperar I/O.
    udp_conn_status_t (*process_events)(const udp_conn_t*);
    size_t (*pending_data)(const udp_conn_t*);
    udp_conn_status_t (*hole_punching)(const udp_conn_t*); // função de hole punching
                                                  // se estiver ausente, será utilizado um padrão (chownat)
    udp_conn_status_t (*connect)(const udp_conn_t*); // fazer o hole-punching
                                            // é um passo antes do connect
                                            // Se conexão não for completa, 
                                            // necessário limpar entrada (porta) na tabela NAT
    udp_conn_status_t (*udp_send)(const udp_conn_t*, void *, size_t, size_t*); // enviar dado para UDP local
    udp_conn_status_t (*udp_recv)(const udp_conn_t*, void*, size_t, size_t*); // precisa ser bufferizado (lista encadeada?)
                                        // acho que não vou bufferizar, ao receber ele faz o recv internamente, 
                                        // e trata assim, mais fácil, o sistema mesmo bufferiza (pode estourar buffer do sistema
                                        // problema de janela do TCP??, analisar mais a fundo, provavelmente era o que dava problema
                                        // na época do aplicativo)
    udp_conn_status_t (*udp_send_ka)(const udp_conn_t*);
    udp_conn_status_t (*disconnect)(const udp_conn_t *); 

    // udp_conn_status_t (*get_reason)(struct udp_conn_t *, void *); 
} udp_conn_generic_api_t;

typedef struct udp_conn {
    udp_transport_t* transport;
    udp_session_t* udp_session;
    tcp_session_t* tcp_session;
    udp_conn_ctrl_t* ctrl;
    udp_conn_generic_api_t* api;
    void (*udp_conn_callback)(const udp_conn_t*, int, void*, size_t); // baseado no callback_websockets do libwebsockets
    void* config;
    void* data;
} udp_conn_t;

int udp_connection(const udp_conn_t *conn);

static inline udp_conn_status_t udp_conn_init(udp_conn_t *conn) {
    if(conn == NULL || conn->transport == NULL || conn->api == NULL || conn->ctrl == NULL)
        return UDP_CONN_ERR;
    if(conn->ctrl->init) return UDP_CONN_ALREADY_INIT;

    if(udp_transport_open(conn->transport) != UDP_TRANSPORT_OK)
        return UDP_CONN_ERR;

    if(conn->udp_session != NULL)
        conn->udp_session->socket_fd = conn->transport->udp_fd;

    udp_conn_status_t status = conn->api->init(conn);
    if(status != UDP_CONN_OK) {
        udp_transport_close(conn->transport);
        if(conn->udp_session != NULL)
            conn->udp_session->socket_fd = -1;
    }

    return status;
}

static inline udp_conn_status_t udp_conn_deinit(udp_conn_t* conn) {
    if(conn == NULL || conn->transport == NULL || conn->api == NULL)
        return UDP_CONN_ERR;

    udp_conn_status_t status = conn->api->deinit(conn);
    tcp_session_t* tcp_session = conn->tcp_session;

    conn->transport->tcp_fd = -1;

    if(tcp_session != NULL) {
        int accepted_sock = tcp_session->accepted_sock;
        int socket_fd = tcp_session->socket_fd;

        if(accepted_sock >= 0 && close(accepted_sock) < 0 && status == UDP_CONN_OK)
            status = UDP_CONN_ERR;

        if(socket_fd >= 0 && socket_fd != accepted_sock &&
           close(socket_fd) < 0 && status == UDP_CONN_OK)
            status = UDP_CONN_ERR;

        tcp_session->accepted_sock = -1;
        tcp_session->socket_fd = -1;
    }

    if(udp_transport_close(conn->transport) != UDP_TRANSPORT_OK &&
       status == UDP_CONN_OK)
        status = UDP_CONN_ERR;

    if(conn->udp_session != NULL)
        conn->udp_session->socket_fd = -1;

    return status;
}

static inline udp_conn_status_t udp_conn_send(const udp_conn_t *conn, void *buf, size_t nbytes, size_t *sent_bytes) {
    return conn->api->udp_send(conn, buf, nbytes, sent_bytes);
}

static inline udp_conn_status_t udp_conn_recv(const udp_conn_t *conn, void *buf, size_t nbytes, size_t *recv_bytes) {
    return conn->api->udp_recv(conn, buf, nbytes, recv_bytes);
}

static inline udp_conn_status_t udp_conn_disconnect(const udp_conn_t *conn) {
    return conn->api->disconnect(conn);
}

#endif