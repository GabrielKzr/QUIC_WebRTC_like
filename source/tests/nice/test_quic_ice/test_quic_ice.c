#include "ice_udp_transport.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ossl_quic.h"

static int signaling_fd;
static gboolean signaling_done;

static int send_all(const void *buffer, size_t length) {
    const char *bytes = buffer;
    while (length > 0) {
        ssize_t sent = send(signaling_fd, bytes, length, MSG_NOSIGNAL);
        if (sent <= 0) return -1;
        bytes += sent;
        length -= (size_t)sent;
    }
    return 0;
}

static int recv_all(void *buffer, size_t length) {
    char *bytes = buffer;
    while (length > 0) {
        ssize_t received = recv(signaling_fd, bytes, length, 0);
        if (received <= 0) return -1;
        bytes += received;
        length -= (size_t)received;
    }
    return 0;
}

static udp_transport_status_t signaling(udp_transport_t *transport, const char *ufrag, const char *password, const GSList *candidates, char **remote_ufrag, char **remote_password, GSList **remote_candidates) {
    ice_udp_transport_data_t *ice = transport->data;
    GString *local = g_string_new(NULL);
    char remote[65537];
    gboolean has_candidate = FALSE;

    g_string_append_printf(local, "a=ice-ufrag:%s\na=ice-pwd:%s\n", ufrag, password);
    for (const GSList *item = candidates; item != NULL; item = item->next) {
        const NiceCandidate *local_candidate = item->data;
#ifdef ICE_REQUIRE_RELAY
        if (local_candidate->type == NICE_CANDIDATE_TYPE_RELAYED)
#else
        if (local_candidate->type == NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE)
#endif
            has_candidate = TRUE;
        char *candidate = nice_agent_generate_local_candidate_sdp(ice->agent, item->data);
        g_string_append_printf(local, "%s\n", candidate);
        g_free(candidate);
    }

    uint32_t frame_length = htonl((uint32_t)local->len);
    gboolean sent = has_candidate && local->len < sizeof(remote) &&
        send_all(&frame_length, sizeof(frame_length)) == 0 &&
        send_all(local->str, local->len) == 0;
    g_string_free(local, TRUE);
    if (!has_candidate) g_printerr("ICE não produziu o tipo de candidato exigido.\n");
    if (!sent) return UDP_TRANSPORT_ERROR;

    if (recv_all(&frame_length, sizeof(frame_length)) < 0) return UDP_TRANSPORT_ERROR;
    uint32_t length = ntohl(frame_length);
    if (length == 0 || length >= sizeof(remote) || recv_all(remote, length) < 0)
        return UDP_TRANSPORT_ERROR;
    remote[length] = '\0';

    *remote_candidates = nice_agent_parse_remote_stream_sdp(
        ice->agent, ice->stream_id, remote, remote_ufrag, remote_password
    );
    signaling_done = *remote_candidates != NULL &&
        *remote_ufrag != NULL && *remote_password != NULL;

    return signaling_done ? UDP_TRANSPORT_OK : UDP_TRANSPORT_ERROR;
}

static ossl_quic_data_t quic_data;
static ossl_quic_config_t quic_config = {
    .tls13_preset = OSSL_TLS13_PRESET_DEFAULT,
    .reuse = 1,
    .cert_file = "./common/certs/server.crt",
    .key_file = "./common/certs/server.key"
};
static udp_session_t udp_session = {.ka_miss_threshold = 20};
static tcp_session_t tcp_session = {.socket_fd = -1, .accepted_sock = -1, .reuse = 1};
static udp_conn_ctrl_t ctrl;
static ice_udp_transport_data_t transport_data;
static ice_udp_transport_config_t ice_config = {.signaling_cb = signaling};
static udp_transport_t transport = {
    .ops = &ice_ops, .tcp_fd = -1, .config = &ice_config, .data = &transport_data
};

#ifdef ICE_LONG_SESSION
#define EXPECTED_BYTES (16384u * 67u)
#else
#define EXPECTED_BYTES 16384u
#endif

static unsigned connected;
static size_t sent_bytes, received_bytes;

static void quic_callback(const udp_conn_t *conn, int reason, void *buffer, size_t length) {
    (void)conn;
    (void)buffer;

    if (reason == OSSL_QUIC_UDP_CONNECTED) connected++;
    if (reason == OSSL_QUIC_UDP_DATA_SENT) sent_bytes += length;
    if (reason == OSSL_QUIC_UDP_RECV_DATA) received_bytes += length;
}

static udp_conn_t conn = {
    .transport = &transport,
    .udp_session = &udp_session,
    .tcp_session = &tcp_session,
    .ctrl = &ctrl,
    .config = &quic_config,
    .data = &quic_data,
    .api = &ossl_quic_api,
    .udp_conn_callback = quic_callback
};

static int wait_ice_ready(void) {
    const struct timeval tick = {.tv_usec = 10000};
    const struct timeval no_wait = {0};
    gint64 deadline = g_get_monotonic_time() + 5 * G_USEC_PER_SEC;

    while (transport_data.state != NICE_COMPONENT_STATE_READY) {
        unsigned events = 0;
        if (g_get_monotonic_time() >= deadline ||
            udp_transport_wait(&transport, UDP_TRANSPORT_READABLE, &tick, &events) != UDP_TRANSPORT_OK) {
            fprintf(stderr, "ICE não chegou a READY.\n");
            return -1;
        }

        if (events & UDP_TRANSPORT_READABLE) {
            unsigned char packet[4];
            size_t length = 0;
            if (udp_transport_recv(&transport, packet, sizeof(packet), &length, &no_wait) != UDP_TRANSPORT_OK ||
                !((length == 1 && packet[0] == 0) ||
                  (length == 3 && memcmp(packet, "01\n", 3) == 0))) {
                return -1;
            }
        }
    }

    NiceCandidate *local = NULL, *remote = NULL;
    if (!signaling_done || !transport_data.gathering_done ||
        !nice_agent_get_selected_pair(transport_data.agent, transport_data.stream_id, 1, &local, &remote)) {
        return -1;
    }
#ifdef ICE_REQUIRE_RELAY
    if (local->type != NICE_CANDIDATE_TYPE_RELAYED || remote->type != NICE_CANDIDATE_TYPE_RELAYED)
        return -1;
#else
    if (local->type == NICE_CANDIDATE_TYPE_RELAYED ||
        (remote->type != NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE &&
         remote->type != NICE_CANDIDATE_TYPE_PEER_REFLEXIVE))
        return -1;
#endif

    char local_ip[NICE_ADDRESS_STRING_LEN], remote_ip[NICE_ADDRESS_STRING_LEN];
    nice_address_to_string(&local->addr, local_ip);
    nice_address_to_string(&remote->addr, remote_ip);
    printf("Peer %c ICE READY: %s:%u -> %s:%u; %s.\n", ctrl.mode == 'c' ? 'A' : 'B',
           local_ip, nice_address_get_port(&local->addr), remote_ip, nice_address_get_port(&remote->addr),
           ice_config.force_relay ? "relay/relay" : "direto/reflexivo");
    udp_session.dst.sin_family = AF_INET;
    udp_session.dst.sin_port = htons(nice_address_get_port(&remote->addr));
    if (inet_pton(AF_INET, remote_ip, &udp_session.dst.sin_addr) != 1) return -1;
    return 0;
}

int main(int argc, char **argv) {
    int debug_level, local_port, remote_port;
    char *mode, *remote_address;
    usage(argc, argv, &debug_level, &mode, &local_port, &remote_address, &remote_port);
    (void)remote_address;
    (void)remote_port;
    (void)debug_level;

    const char *isp = getenv("PATCHBAY_ISP_ADDRESS");
    ice_config.local_address = getenv("PATCHBAY_LOCAL_ADDRESS");
    if (isp == NULL || ice_config.local_address == NULL) {
        fprintf(stderr, "Execute pelo runner Patchbay com serviços no ISP.\n");
        return EXIT_FAILURE;
    }

    ctrl.mode = mode[1];
    ice_config.controlling = ctrl.mode == 'c';
    ice_config.stun_server = isp;
    ice_config.stun_port = 3478;
#ifdef ICE_REQUIRE_RELAY
    ice_config.turn_server = isp;
    ice_config.turn_port = 3478;
    ice_config.turn_username = "ice-test";
    ice_config.turn_password = "ice-test-password";
    ice_config.force_relay = TRUE;
#endif

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_port = htons(5555)};
    if (inet_pton(AF_INET, isp, &server.sin_addr) != 1) return EXIT_FAILURE;
    signaling_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (signaling_fd < 0) return EXIT_FAILURE;
    int result = EXIT_FAILURE;
    gboolean initialized = FALSE;
    const struct timeval timeout = {.tv_sec = 3};

    if (setsockopt(signaling_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(signaling_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0 ||
        connect(signaling_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("signaling TCP");
        goto cleanup;
    }

    tcp_session.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_session.socket_fd < 0) goto cleanup;
    tcp_session.local.sin_family = AF_INET;
    tcp_session.local.sin_port = htons(local_port);
    tcp_session.local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (udp_conn_init(&conn) != UDP_CONN_OK) goto cleanup;
    initialized = TRUE;
    if (wait_ice_ready() < 0) goto cleanup;

    udp_conn_status_t status = udp_connection(&conn);
    if (status != UDP_CONN_OK || connected != 1 || !quic_data.closed ||
        sent_bytes != EXPECTED_BYTES || received_bytes != EXPECTED_BYTES) {
        fprintf(stderr, "QUIC/ICE incompleto: status=%d connected=%u sent=%zu received=%zu closed=%d.\n",
                status, connected, sent_bytes, received_bytes, quic_data.closed);
        goto cleanup;
    }
    result = EXIT_SUCCESS;

cleanup:
    if (initialized) {
        if (udp_conn_deinit(&conn) != UDP_CONN_OK || ctrl.init || transport.opened ||
            transport_data.agent != NULL || transport_data.context != NULL ||
            tcp_session.socket_fd != -1 || tcp_session.accepted_sock != -1) {
            result = EXIT_FAILURE;
        }
    } else if (tcp_session.socket_fd >= 0) {
        close(tcp_session.socket_fd);
    }
#ifdef ICE_REQUIRE_RELAY
    if (result == EXIT_SUCCESS) {
        uint32_t length = htonl(6);
        char reply[2];
        if (send_all(&length, sizeof(length)) < 0 || send_all("CLOSED", 6) < 0 ||
            recv_all(reply, sizeof(reply)) < 0 || memcmp(reply, "OK", 2) != 0) {
            result = EXIT_FAILURE;
        }
    }
#endif
    close(signaling_fd);
    if (result == EXIT_SUCCESS) {
        printf("Peer %c QUIC/ICE: handshake, %u bytes nos dois sentidos e fechamento completos.\n",
               ctrl.mode == 'c' ? 'A' : 'B', EXPECTED_BYTES);
    } else {
        fprintf(stderr, "Peer %c falhou no teste QUIC/ICE.\n", ctrl.mode == 'c' ? 'A' : 'B');
    }
    return result;
}
