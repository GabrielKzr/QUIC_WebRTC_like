#include "ice_udp_transport.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *scenario;
static guint16 unused_port;
static int signaling_calls;
static int signaling_refused;
static int remote_configured;
static int relay_candidates;

static void warm_closed(GObject *source, GAsyncResult *result, gpointer user_data) {
    (void)source;
    (void)result;
    *(gboolean *)user_data = TRUE;
}

static int fd_count(void) {
    DIR *directory = opendir("/proc/self/fd");
    if (directory == NULL) return -1;
    int count = 0;
    while (readdir(directory) != NULL) count++;
    closedir(directory);
    return count;
}

static int reserve_port(int type, guint16 *port) {
    int fd = socket(AF_INET, type, 0);
    struct sockaddr_in address = {.sin_family = AF_INET};
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t length = sizeof(address);
    if (fd < 0) return -1;
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        getsockname(fd, (struct sockaddr *)&address, &length) < 0) {
        close(fd);
        return -1;
    }
    *port = ntohs(address.sin_port);
    return fd;
}

static udp_transport_status_t signaling(udp_transport_t *transport,
        const char *ufrag, const char *password, const GSList *candidates,
        char **remote_ufrag, char **remote_password, GSList **remote_candidates) {
    (void)transport;
    (void)ufrag;
    (void)password;
    signaling_calls++;
    for (const GSList *item = candidates; item != NULL; item = item->next) {
        const NiceCandidate *candidate = item->data;
        if (candidate->type == NICE_CANDIDATE_TYPE_RELAYED) relay_candidates++;
    }

    if (strcmp(scenario, "signaling_unavailable") == 0) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(unused_port)};
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (fd < 0) return UDP_TRANSPORT_ERROR;
        signaling_refused = connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0 &&
            errno == ECONNREFUSED;
        close(fd);
        return UDP_TRANSPORT_ERROR;
    }

    NiceCandidate *remote = nice_candidate_new(NICE_CANDIDATE_TYPE_HOST);
    remote->stream_id = 1;
    remote->component_id = 1;
    remote->transport = NICE_CANDIDATE_TRANSPORT_UDP;
    remote->priority = 1;
    g_strlcpy(remote->foundation, "absent", sizeof(remote->foundation));
    nice_address_set_from_string(&remote->addr, "127.0.0.1");
    nice_address_set_port(&remote->addr, unused_port);
    remote->base_addr = remote->addr;
    *remote_ufrag = g_strdup("absent");
    *remote_password = g_strdup("absent-peer-password-123456");
    *remote_candidates = g_slist_append(NULL, remote);
    remote_configured = 1;
    return UDP_TRANSPORT_OK;
}

static GSubprocess *start_turn(guint16 port, const char *log_path) {
    char port_arg[32];
    g_snprintf(port_arg, sizeof(port_arg), "--listening-port=%u", port);
    GSubprocessLauncher *launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_STDERR_MERGE);
    g_subprocess_launcher_set_stdout_file_path(launcher, log_path);
    GError *error = NULL;
    GSubprocess *server = g_subprocess_launcher_spawn(launcher, &error,
        "turnserver", "-n", "--verbose", "--listening-ip=127.0.0.1",
        "--relay-ip=127.0.0.1", port_arg, "--relay-threads=1", "--lt-cred-mech",
        "--realm=ice-test", "--user=ice-test:correct-password",
        strcmp(scenario, "turn_stale_nonce") == 0 ? "--stale-nonce=1" : "--stale-nonce=600",
        "--no-tcp", "--no-tls", "--no-dtls", "--no-cli",
        "--log-file=stdout", "--pidfile=/dev/null", NULL);
    if (error != NULL) {
        fprintf(stderr, "TURN: %s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(launcher);
    if (server == NULL) return NULL;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval timeout = {.tv_usec = 50000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(port)};
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    const unsigned char request[20] = {0, 1, 0, 0, 0x21, 0x12, 0xa4, 0x42,
                                       1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    gint64 deadline = g_get_monotonic_time() + 2 * G_USEC_PER_SEC;
    gboolean ready = FALSE;
    while (fd >= 0 && g_get_monotonic_time() < deadline) {
        unsigned char reply[256];
        sendto(fd, request, sizeof(request), 0, (struct sockaddr *)&address, sizeof(address));
        ssize_t length = recv(fd, reply, sizeof(reply), 0);
        if (length >= 20 && reply[0] == 1 && reply[1] == 1 &&
            memcmp(reply + 4, request + 4, 16) == 0) {
            ready = TRUE;
            break;
        }
    }
    if (fd >= 0) close(fd);
    if (!ready) {
        g_subprocess_force_exit(server);
        g_subprocess_wait(server, NULL, NULL);
        g_clear_object(&server);
    }
    return server;
}

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    scenario = argv[1];
    gboolean bad_turn = strcmp(scenario, "turn_bad_credentials") == 0;
    gboolean stale_nonce = strcmp(scenario, "turn_stale_nonce") == 0;
    if (!bad_turn && !stale_nonce && strcmp(scenario, "remote_absent") != 0 &&
        strcmp(scenario, "signaling_unavailable") != 0) return 1;

    int reserved = reserve_port(strcmp(scenario, "signaling_unavailable") == 0 ?
                                SOCK_STREAM : SOCK_DGRAM, &unused_port);
    if (reserved < 0) return 1;
    GSubprocess *server = NULL;
    char *log_path = NULL;
    ice_udp_transport_config_t config = {
        .local_address = "127.0.0.1", .controlling = TRUE, .signaling_cb = signaling
    };
    int result = 1;
    if (bad_turn || stale_nonce) {
        guint16 port;
        int fd = reserve_port(SOCK_DGRAM, &port);
        if (fd < 0) goto cleanup;
        close(fd);
        fd = g_file_open_tmp("ice-failure-turn-XXXXXX", &log_path, NULL);
        if (fd < 0) goto cleanup;
        close(fd);
        server = start_turn(port, log_path);
        if (server == NULL) goto cleanup;
        config.turn_server = "127.0.0.1";
        config.turn_port = port;
        config.turn_username = "ice-test";
        config.turn_password = bad_turn ? "wrong-password" : "correct-password";
        config.force_relay = TRUE;
    }

    ice_udp_transport_data_t data = {0};
    udp_transport_t transport = {
        .ops = &ice_ops, .tcp_fd = -1, .config = &config, .data = &data
    };
    /* Inicializar também os recursos globais de GTask antes de contar FDs. */
    GMainContext *context = g_main_context_default();
    g_main_context_iteration(context, FALSE);
    NiceAgent *warm_agent = nice_agent_new(context, NICE_COMPATIBILITY_RFC5245);
    gboolean warm_done = FALSE;
    nice_agent_close_async(warm_agent, warm_closed, &warm_done);
    while (!warm_done) g_main_context_iteration(context, TRUE);
    g_object_unref(warm_agent);
    int before = fd_count();
    gint64 started = g_get_monotonic_time();
    if (udp_transport_open(&transport) != UDP_TRANSPORT_OK) goto cleanup;
    /* Prazos curtos exclusivos do teste; não alteram os defaults da biblioteca. */
    g_object_set(data.agent, "stun-initial-timeout", 50u,
                 "stun-max-retransmissions", 2u, "idle-timeout", 100u, NULL);
    const struct timeval tick = {.tv_usec = 10000};
    udp_transport_status_t status = UDP_TRANSPORT_OK;
    while (g_get_monotonic_time() - started < 8 * G_USEC_PER_SEC) {
        unsigned events = 0;
        status = udp_transport_wait(&transport, UDP_TRANSPORT_READABLE, &tick, &events);
        if (status != UDP_TRANSPORT_OK || data.state == NICE_COMPONENT_STATE_READY) break;
    }
    gboolean expected = status == UDP_TRANSPORT_ERROR &&
        data.state == NICE_COMPONENT_STATE_FAILED;
    if (strcmp(scenario, "signaling_unavailable") == 0)
        expected = expected && signaling_calls == 1 && signaling_refused;
    else if (bad_turn)
        expected = expected && relay_candidates == 0 && config.force_relay;
    else
        expected = expected && signaling_calls == 1 && remote_configured;

    if (stale_nonce) {
        expected = expected && relay_candidates == 1 && config.force_relay;
        /* Expirar o nonce antes de cancelar a allocation do peer ausente. */
        g_usleep(2200000);
    }

    gint64 elapsed = g_get_monotonic_time() - started;
    if (udp_transport_close(&transport) != UDP_TRANSPORT_OK || transport.opened ||
        data.agent != NULL || data.context != NULL || before < 0 || fd_count() != before) expected = FALSE;
    if (bad_turn) {
        char *log = NULL;
        g_file_get_contents(log_path, &log, NULL, NULL);
        expected = expected && log != NULL && strstr(log, "401") != NULL &&
            strstr(log, "incoming packet ALLOCATE processed, success") == NULL;
        g_free(log);
    }
    if (stale_nonce) {
        gboolean released = FALSE;
        gint64 deadline = g_get_monotonic_time() + 2 * G_USEC_PER_SEC;
        do {
            char *log = NULL;
            g_file_get_contents(log_path, &log, NULL, NULL);
            released = log != NULL && strstr(log, "438: Stale nonce") != NULL &&
                strstr(log, "refreshed, realm=<ice-test>, username=<ice-test>, lifetime=0") != NULL &&
                strstr(log, ": delete: realm=<ice-test>, username=<ice-test>") != NULL;
            g_free(log);
            if (!released) g_usleep(10000);
        } while (!released && g_get_monotonic_time() < deadline);
        expected = expected && released;
        printf("TURN stale nonce: %s\n", released ? "server removal confirmed" : "FAILED");
    }
    printf("%s: %s; status=%d; elapsed=%.3f s\n", scenario,
           expected ? "expected failure and cleanup confirmed" : "FAILED",
           status, elapsed / (double)G_USEC_PER_SEC);
    result = expected ? 0 : 1;

cleanup:
    if (server != NULL) {
        g_subprocess_force_exit(server);
        g_subprocess_wait(server, NULL, NULL);
        g_object_unref(server);
    }
    if (log_path != NULL) {
        unlink(log_path);
        g_free(log_path);
    }
    close(reserved);
    return result;
}
