#ifndef CHOWNAT_H
#define CHOWNAT_H

#include "udp_conn.h"

#define size 1024 // < MTU (1500) -- Se fragmentar, pode dar problema

enum chownat_reasons {
    CHOWNAT_UDP_CONNECTED, // triggered after connected
    CHOWNAT_UDP_RECV_DATA, // triggered after receiveing data on udp
    CHOWNAT_UDP_LOST_DATA, // triggered after retransmiting lost data
    CHOWNAT_TCP_DATA_SENT, // triggered after sending service data through udp conn
    CHOWNAT_UDP_DATA_SENT, // triggered after sending data through udp conn
};

typedef struct {
    int closed;
    int busy;
    int id;
    int expected;
    char buffer[256][size];
    size_t sizes[256];
} chownat_data_t;

typedef struct {
    int udp_recv_timeout_sec;
    int reuse;
    int conn_max_attempts; // máximo de tentativas de conexão
    int dconn_max_attempts; // máximo de tentativas de desconexão
} chownat_config_t;

extern struct udp_conn_generic_api_t chownat_api;

#endif