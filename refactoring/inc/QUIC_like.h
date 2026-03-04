#ifndef QUIC_LIKE_H
#define QUIC_LIKE_H

#include "udp_conn.h"

#define QUIC_LONG_HEADER_TYPE 0xC0 // 1100 0000
#define QUIC_SHORT_HEADER_TYPE 0x40 // 0100 0000

enum long_header_packet_t {
    QUIC_INITIAL   = 0x00,    // 0000 0000
    QUIC_0RTT      = 0x10,    // 0001 0000
    QUIC_HANDSHAKE = 0x20,    // 0010 0000
    QUIC_RETRY     = 0x30     // 0011 0000
};

/*
    * First Byte lower bits (0x0f): 

        - Initial:    | reserved (2) | packet number bytes (2) |
        - 0-rtt:      | reserved (2) | packet number bytes (2) |
        - handshake:  | reserved (2) | packet number bytes (2) |
        - retry:      | unused (4) |

*/

#define QUIC_LIKE_VERSION 0xebdd0001

struct quic_long_header_packet_t {
    uint8_t first_byte;
    uint32_t quic_version;
    uint8_t d_connection_id_len; // na prática não vai ser usado
    uint8_t d_connection_id[20]; // na prática não vai ser usado
    uint8_t s_connection_id_len;
    uint8_t s_connection_id[20];
    void* payload;
};

#endif