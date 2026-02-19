#ifndef UDP_CONN_H
#define UDP_CONN_H

#include <sys/select.h>
#include <unistd.h>
#include "utils.h"

#define localhost "127.0.0.1"

struct udp_conn_t {
    char* name;
    struct udp_conn_session_t* session;
    void* config;
    void* data;
    // void* reasons;
    struct udp_conn_generic_api_t* api;
    void (*udp_conn_callback)(const struct udp_conn_t*, int, void*, size_t); // baseado no callback_websockets do libwebsockets
                                                                     // Parametros: conn, reason, data_in (buffer), len (tamanho do data_in)
    struct tcp_tunneling_t* tcp_tun;
};

struct udp_conn_session_t {
    int socket_fd;
    char mode;
    int ka_miss_threshold;
    struct sockaddr_in dst;
    struct sockaddr_in src;  
};

// baseado em callback, então as funções abaixo serão executadas quando 
// o protocolo for executar uma dessas operações
// essas funções são definidas previamente e chamadas quando uma dessas
// operações for realizadas
// A operação task é a operação responsável por gerenciar estados após conexão
struct udp_conn_generic_api_t {
    int (*init)(const struct udp_conn_t*);
    int (*deinit)(const struct udp_conn_t*);
    int (*hole_punching)(const struct udp_conn_t*); // função de hole punching
                                                  // se estiver ausente, será utilizado um padrão (chownat)
    int (*connect)(const struct udp_conn_t*); // fazer o hole-punching
                                            // é um passo antes do connect
                                            // Se conexão não for completa, 
                                            // necessário limpar entrada (porta) na tabela NAT
    size_t (*udp_send)(const struct udp_conn_t*, void *, size_t); // enviar dado para UDP local
    size_t (*udp_recv)(const struct udp_conn_t*); // precisa ser bufferizado (lista encadeada?)
                                        // acho que não vou bufferizar, ao receber ele faz o recv internamente, 
                                        // e trata assim, mais fácil, o sistema mesmo bufferiza
    int (*udp_send_ka)(const struct udp_conn_t*);
    int (*disconnect)(const struct udp_conn_t *); 

    // int (*get_reason)(struct udp_conn_t *, void *); 

    // handling de tunneling de TCP
    int (*tcp_bind)(const struct udp_conn_t*);
    int (*tcp_recv)(const struct udp_conn_t*);
};

int udp_conn_init(struct udp_conn_t *conn); // essa função podia estar internamente no udp_connection, 
                                            // mas está fora do loop principal no fluxo conceitual proposto pelo chownat
                                            // portanto ficou fora, e também acaba servindo como uma confirmação de sucesso
                                            // de inicialização para o usuário
int udp_conn_deinit(struct udp_conn_t* conn);
int udp_connection(const struct udp_conn_t *conn);
size_t udp_conn_send(const struct udp_conn_t *conn, void *data, size_t nbytes); // função liberada pra usar na callback
size_t udp_conn_recv(const struct udp_conn_t *conn); // função liberada pra usar na callback
int udp_conn_disconnect(const struct udp_conn_t *conn); // função liberada pra usar na callback

extern int initiated;
extern int closed;

struct tcp_tunneling_t {
    int socket_fd;
    int accepted_sock;
    struct sockaddr_in local;
    int tcp_recv_timeout_sec;
    int reuse; 
};

#endif