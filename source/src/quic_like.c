#include "quic_like.h"

/*
static void *quicl_build_long_initial(uint8_t first_byte, uint32_t quic_version, 
                                uint8_t d_connection_id_len, uint8_t *d_connection_id, 
                                uint8_t s_connection_id_len, uint8_t *s_connection_id, void *type_specific_payload);

static void *quicl_build_packet(uint8_t first_byte, uint32_t quic_version, 
                                uint8_t d_connection_id_len, uint8_t *d_connection_id, 
                                uint8_t s_connection_id_len, uint8_t *s_connection_id, void *type_specific_payload) {

    uint8_t header_type = first_byte & QUIC_LONG_HEADER_TYPE;

    if(header_type == QUIC_LONG_HEADER_TYPE) {

        uint8_t packet_type = first_byte & (0x30 << 4);

        switch (packet_type)
        {
        case QUIC_INITIAL:
            
            return quicl_build_long_initial(first_byte, quic_version, d_connection_id_len, d_connection_id, s_connection_id_len, s_connection_id, type_specific_payload);
            
        case QUIC_0RTT:

            break;

        case QUIC_HANDSHAKE:

            break;

        case QUIC_RETRY:

            break;
     
        default:
            break;
        }


    } else if (header_type == QUIC_SHORT_HEADER_TYPE){

        // ainda precisa ser implementado

    } else {
        DEBUG_PRINT("[ERROR] Unknown header type");        
    }
}
*/