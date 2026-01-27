#ifndef UDP_CONN_H
#define UDP_CONN_H

#include "utils.h"

#define localhost "127.0.0.1"

struct udp_conn_t {
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
struct udp_conn_generic_api {
    uint8_t (*connect)(struct udp_conn_t*); // fazer o hole-punching
                                            // é um passo antes do connect
                                            // Se conexão não for completa, 
                                            // necessário limpar entrada (porta) na tabela NAT
    uint8_t (*send)(struct udp_conn_t*, void *); // enviar dado para UDP local
    uint8_t (*recv)(struct udp_conn_t*, void *); // precisa ser bufferizado (lista encadeada?)
    uint8_t (*disconnect)(struct udp_conn_t *, void *); 
    uint8_t (*task)(struct udp_conn_t *, void *); // task será responsável por chamar send e recv
};

uint8_t udp_conn_init(struct udp_conn_t *conn, int socket_fd, char mode, int remoteport, char* remoteaddr);
uint8_t udp_conn_start(struct udp_conn_t *conn);



#endif