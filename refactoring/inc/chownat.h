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
    int conn_sec_timeout;
};

extern const struct udp_conn_generic_api_t chownat_api;

#endif