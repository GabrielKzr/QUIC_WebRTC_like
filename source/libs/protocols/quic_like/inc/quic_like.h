#ifndef QUIC_LIKE_H
#define QUIC_LIKE_H

#include "udp_conn.h"

#define INITIAL_SALT 0x38762cf7f55934b34d179ae6a4c80cadccbb7f0a // 160 bits (20 bytes)

#define QUIC_LONG_HEADER_TYPE 0xC0 // 1100 0000
#define QUIC_SHORT_HEADER_TYPE 0x40 // 0100 0000

#define STREAM_MAX_DATA 1024 // fixed (diferente do RFC)

enum long_header_packet_t {
    QUIC_INITIAL   = 0x00,    // 0000 0000
    QUIC_0RTT      = 0x10,    // 0001 0000
    QUIC_HANDSHAKE = 0x20,    // 0010 0000
    QUIC_RETRY     = 0x30     // 0011 0000
};

enum stream_state_s {
    OPEN,
    HALF_CLOSED,
    CLOSED,
    RESET
};

struct basic_stream_t {
    uint64_t stream_id;
    uint64_t send_offset;
    uint64_t recv_offset;  
    enum stream_state_s state;
};

/*
    * First Byte lower bits (0x0f): 

        - Initial:    | reserved (2) | packet number bytes (2) |
        - 0-rtt:      | reserved (2) | packet number bytes (2) |
        - handshake:  | reserved (2) | packet number bytes (2) |
        - retry:      | unused (4) |

*/

#define QUIC_LIKE_VERSION 0xebdd0001

/*
struct quic_long_header_packet_t {
    uint8_t first_byte;
    uint32_t quic_version;
    uint8_t d_connection_id_len; // na prática não vai ser usado
    uint8_t d_connection_id[20]; // na prática não vai ser usado
    uint8_t s_connection_id_len;
    uint8_t s_connection_id[20];
    void* payload;
};
*/

#endif