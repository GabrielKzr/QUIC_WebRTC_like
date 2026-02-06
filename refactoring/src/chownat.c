#include "chownat.h"





const struct udp_conn_generic_api_t chownat_api = {
    .init = NULL,
    .deinit = NULL,
    .hole_punching = NULL,
    .connect = NULL,
    .udp_send = NULL,
    .udp_recv = NULL,
    .udp_send_ka = NULL,
    .disconnect = NULL,
    .tcp_bind = NULL,
    .tcp_recv = NULL
};