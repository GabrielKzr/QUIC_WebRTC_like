#include "ice_udp_transport.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
    gboolean has_srflx = FALSE;

    g_string_append_printf(local, "a=ice-ufrag:%s\na=ice-pwd:%s\n", ufrag, password);
    for (const GSList *item = candidates; item != NULL; item = item->next) {
        const NiceCandidate *local_candidate = item->data;
        if (local_candidate->type == NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE)
            has_srflx = TRUE;
        char *candidate = nice_agent_generate_local_candidate_sdp(ice->agent, item->data);
        g_string_append_printf(local, "%s\n", candidate);
        g_free(candidate);
    }

    uint32_t frame_length = htonl((uint32_t)local->len);
    gboolean sent = has_srflx && local->len < sizeof(remote) &&
        send_all(&frame_length, sizeof(frame_length)) == 0 &&
        send_all(local->str, local->len) == 0;
    g_string_free(local, TRUE);
    if (!has_srflx) g_printerr("STUN não produziu candidato server-reflexive.\n");
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

static int run_peer(const char *isp_address, gboolean controlling) {
    ice_udp_transport_data_t data = {0};
    ice_udp_transport_config_t config = {
        .controlling = controlling,
        .local_address = getenv("PATCHBAY_LOCAL_ADDRESS"),
        .stun_server = isp_address,
        .stun_port = 3478,
        .signaling_cb = signaling
    };
    udp_transport_t transport = {
        .ops = &ice_ops, .tcp_fd = -1, .config = &config, .data = &data
    };
    const char *message = controlling ? "mensagem do peer A" : "mensagem do peer B";
    const char *expected = controlling ? "mensagem do peer B" : "mensagem do peer A";
    const struct timeval tick = {.tv_usec = 10000};
    const struct timeval zero = {0};
    gint64 deadline = g_get_monotonic_time() + 5 * G_USEC_PER_SEC;
    gboolean sent = FALSE, received = FALSE;
    int result = EXIT_FAILURE;

    if (udp_transport_open(&transport) != UDP_TRANSPORT_OK) goto cleanup;

    while (!sent || !received) {
        if (g_get_monotonic_time() >= deadline) {
            g_printerr("Timeout no transporte ICE através dos NATs.\n");
            goto cleanup;
        }

        unsigned requested = UDP_TRANSPORT_READABLE;
        unsigned events = 0;
        if (!sent) requested |= UDP_TRANSPORT_WRITABLE;
        if (udp_transport_wait(&transport, requested, &tick, &events) != UDP_TRANSPORT_OK)
            goto cleanup;

        if (!sent && (events & UDP_TRANSPORT_WRITABLE)) {
            udp_transport_status_t status = udp_transport_send(
                &transport, message, strlen(message), &zero
            );
            if (status == UDP_TRANSPORT_ERROR) goto cleanup;
            sent = status == UDP_TRANSPORT_OK;
        }

        if (events & UDP_TRANSPORT_READABLE) {
            char buffer[64];
            size_t length = 0;
            if (udp_transport_recv(&transport, buffer, sizeof(buffer), &length, &zero) != UDP_TRANSPORT_OK)
                goto cleanup;
            if (received || length != strlen(expected) || memcmp(buffer, expected, length) != 0) {
                g_printerr("Datagrama inesperado no transporte ICE.\n");
                goto cleanup;
            }
            received = TRUE;
        }
    }

    if (!signaling_done || !data.gathering_done || data.state != NICE_COMPONENT_STATE_READY)
        goto cleanup;

    NiceCandidate *local = NULL, *remote = NULL;
    if (!nice_agent_get_selected_pair(data.agent, data.stream_id, 1, &local, &remote) ||
        (remote->type != NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE &&
         remote->type != NICE_CANDIDATE_TYPE_PEER_REFLEXIVE)) {
        g_printerr("ICE não selecionou um candidato remoto reflexivo.\n");
        goto cleanup;
    }
    char local_ip[NICE_ADDRESS_STRING_LEN], remote_ip[NICE_ADDRESS_STRING_LEN];
    nice_address_to_string(&local->addr, local_ip);
    nice_address_to_string(&remote->addr, remote_ip);
    g_print("Peer %c READY: %s:%u -> %s:%u; mensagens corretas nos dois sentidos.\n",
            controlling ? 'A' : 'B', local_ip, nice_address_get_port(&local->addr),
            remote_ip, nice_address_get_port(&remote->addr));
    result = EXIT_SUCCESS;

cleanup:
    if (udp_transport_close(&transport) != UDP_TRANSPORT_OK ||
        transport.opened || data.agent != NULL || data.context != NULL) {
        result = EXIT_FAILURE;
    }
    if (result != EXIT_SUCCESS)
        g_printerr("Peer %c falhou no teste ICE/NAT.\n", controlling ? 'A' : 'B');
    return result;
}

int main(int argc, char **argv) {
    int arg = 1;
    if (arg < argc && strcmp(argv[arg], "-d") == 0) arg++;
    if (argc - arg != 2 ||
        (strcmp(argv[arg], "-c") != 0 && strcmp(argv[arg], "-s") != 0)) {
        g_printerr("Uso: %s [-d] <-c|-s> <remote-address>\n", argv[0]);
        return EXIT_FAILURE;
    }
    gboolean controlling = strcmp(argv[arg], "-c") == 0;
    const char *isp_address = getenv("PATCHBAY_ISP_ADDRESS");
    if (isp_address == NULL || getenv("PATCHBAY_LOCAL_ADDRESS") == NULL) {
        g_printerr("Execute este teste pelo runner Patchbay (endereços ISP e local).\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_port = htons(5555)};
    if (inet_pton(AF_INET, isp_address, &server.sin_addr) != 1) return EXIT_FAILURE;
    signaling_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (signaling_fd < 0) return EXIT_FAILURE;
    const struct timeval timeout = {.tv_sec = 3};
    if (setsockopt(signaling_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(signaling_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0 ||
        connect(signaling_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("signaling TCP");
        close(signaling_fd);
        return EXIT_FAILURE;
    }

    int result = run_peer(isp_address, controlling);
    close(signaling_fd);
    return result;
}
