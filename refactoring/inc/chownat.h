#ifndef CHOWNAT_H
#define CHOWNAT_H

#include "udp_conn.h"

#define size 1024 // < MTU (1500) -- Se fragmentar, pode dar problema

struct chownat_data_t {
    int busy;
    int id;
    int expected;
    char buffer[256][size];
    size_t sizes[256];
};

struct chownat_config_t {
    int udp_recv_timeout_sec;
    int reuse;
    int conn_max_attempts; // máximo de tentativas de conexão
    int dconn_max_attempts; // máximo de tentativas de desconexão
};

extern struct udp_conn_generic_api_t chownat_api;

#endif