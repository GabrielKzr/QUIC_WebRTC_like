#ifndef UDP_CONN_H
#define UDP_CONN_H

#include <sys/select.h>
#include <limits.h>
#include "utils.h"

#define localhost "127.0.0.1"

#define TCP_RECEIVE INT_MAX // um enum não vai chegar nesse número
                            // mas por definição, enums só podem ir até INT_MAX-1
                            // ao utilizar udp_conn_callback

struct udp_conn_t {
    char* name;
    struct udp_conn_session_t* session;
    void* config;
    void* data;
    void* reasons;
    struct udp_conn_generic_api_t* api;
    int (*udp_conn_callback)(struct udp_conn_t*, int, void*, size_t); // baseado no callback_websockets do libwebsockets
                                                                     // Parametros: conn, reason, data_in (buffer), len (tamanho do data_in)
    struct tcp_tunneling_t* tcp_tun;
};

struct udp_conn_session_t {
    int socket_fd;
    char mode;
    struct sockaddr_in dst;
    struct sockaddr_in src;  
};

// baseado em callback, então as funções abaixo serão executadas quando 
// o protocolo for executar uma dessas operações
// essas funções são definidas previamente e chamadas quando uma dessas
// operações for realizadas
// A operação task é a operação responsável por gerenciar estados após conexão
struct udp_conn_generic_api_t {
    int (*init)(struct udp_conn_t*);
    int (*hole_punching)(struct udp_conn_t*); // função de hole punching
                                                  // se estiver ausente, será utilizado um padrão (chownat)
    int (*connect)(struct udp_conn_t*); // fazer o hole-punching
                                            // é um passo antes do connect
                                            // Se conexão não for completa, 
                                            // necessário limpar entrada (porta) na tabela NAT
    size_t (*udp_send)(struct udp_conn_t*, void *); // enviar dado para UDP local
    size_t (*udp_recv)(struct udp_conn_t*); // precisa ser bufferizado (lista encadeada?)
                                        // acho que não vou bufferizar, ao receber ele faz o recv internamente, 
                                        // e trata assim, mais fácil, o sistema mesmo bufferiza
    int (*disconnect)(struct udp_conn_t *, struct timeval*); 

    int (*get_reason)(struct udp_conn_t *, void *); // task será responsável por chamar send e recv

    // handling de tunneling de TCP
    int (*tcp_client_bind)(struct udp_conn_t*);
    int (*tcp_recv)(struct udp_conn_t*);
};

int udp_conn_init(struct udp_conn_t *conn); // essa função podia estar internamente no udp_connection, 
                                            // mas está fora do loop principal no fluxo conceitual proposto pelo chownat
                                            // portanto ficou fora, e também acaba servindo como uma confirmação de sucesso
                                            // de inicialização para o usuário

int udp_connection(struct udp_conn_t *conn);

extern int initiated;
extern int closed;

struct tcp_tunneling_t {
    int socket_fd;
    int accepted_sock;
    struct sockaddr_in local;
    int reuse; 
};

#endif