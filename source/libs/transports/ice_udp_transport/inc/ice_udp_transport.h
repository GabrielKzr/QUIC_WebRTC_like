#ifndef ICE_UDP_TRANSPORT_H
#define ICE_UDP_TRANSPORT_H

#include "udp_transport.h"
#include <nice/agent.h>

typedef udp_transport_status_t (*udp_transport_signaling_cb_t)(udp_transport_t *transport, const char *ufrag, const char *password, const GSList *candidates, char **remote_ufrag, char **remote_password, GSList **remote_candidates);

typedef struct {
    GMainContext *context;
    NiceAgent *agent;
    guint stream_id;
    NiceComponentState state;
    gboolean gathering_done;

    unsigned char pending_packet[65536];
    size_t pending_size;
    gboolean has_pending;
} ice_udp_transport_data_t;

typedef struct {
    gboolean controlling;
    const char *local_address;

    const char *stun_server;
    guint stun_port;

    const char *turn_server;
    guint turn_port;
    const char *turn_username;
    const char *turn_password;
    gboolean force_relay;

    udp_transport_signaling_cb_t signaling_cb;
} ice_udp_transport_config_t;

extern const udp_transport_ops_t ice_ops;

#endif
