#include <nice/agent.h>
#include <stdlib.h>
#include <string.h>

typedef struct test test_t;

typedef struct {
    test_t *test;
    NiceAgent *agent;
    guint stream;
    gboolean gathered;
    gboolean ready;
    gboolean received;
    const char *message;
} peer_t;

struct test {
    GMainLoop *loop;
    peer_t peers[2];
    gboolean failed;
};

static void gathering_done(NiceAgent *agent, guint stream, gpointer user_data) {
    peer_t *peer = user_data;
    test_t *test = peer->test;
    (void)agent;
    (void)stream;

    peer->gathered = TRUE;
    if (test->peers[0].gathered && test->peers[1].gathered)
        g_main_loop_quit(test->loop);
}

static void state_changed(NiceAgent *agent, guint stream, guint component,
                          guint state, gpointer user_data) {
    peer_t *peer = user_data;
    test_t *test = peer->test;
    (void)agent;
    (void)stream;
    (void)component;

    if (state == NICE_COMPONENT_STATE_FAILED) {
        g_printerr("ICE falhou.\n");
        test->failed = TRUE;
        g_main_loop_quit(test->loop);
    }

    peer->ready = state == NICE_COMPONENT_STATE_READY;
    if (test->peers[0].ready && test->peers[1].ready)
        g_main_loop_quit(test->loop);
}

static void receive_data(NiceAgent *agent, guint stream, guint component,
                         guint length, gchar *buffer, gpointer user_data) {
    peer_t *peer = user_data;
    test_t *test = peer->test;
    const char *expected = peer == &test->peers[0]
        ? test->peers[1].message : test->peers[0].message;
    (void)agent;
    (void)stream;
    (void)component;

    if (peer->received || length != strlen(expected) ||
        memcmp(buffer, expected, length) != 0) {
        g_printerr("Datagrama inesperado.\n");
        test->failed = TRUE;
        g_main_loop_quit(test->loop);
        return;
    }

    peer->received = TRUE;
    g_print("Recebido: %.*s\n", (int)length, buffer);
    if (test->peers[0].received && test->peers[1].received)
        g_main_loop_quit(test->loop);
}

static gboolean timeout_expired(gpointer user_data) {
    test_t *test = user_data;

    g_printerr("Timeout no teste local da libnice.\n");
    test->failed = TRUE;
    g_main_loop_quit(test->loop);
    return G_SOURCE_CONTINUE;
}

int main(void) {
    test_t test = {0};
    NiceAddress local;
    int result = EXIT_FAILURE;
    guint timeout;

    test.loop = g_main_loop_new(NULL, FALSE);
    timeout = g_timeout_add_seconds(5, timeout_expired, &test);
    test.peers[0].message = "mensagem do peer A";
    test.peers[1].message = "mensagem do peer B";

    nice_address_init(&local);
    if (!nice_address_set_from_string(&local, "127.0.0.1")) goto cleanup;

    for (int i = 0; i < 2; i++) {
        peer_t *peer = &test.peers[i];

        peer->test = &test;
        peer->agent = nice_agent_new(NULL, NICE_COMPATIBILITY_RFC5245);
        if (peer->agent == NULL) goto cleanup;

        g_object_set(peer->agent, "controlling-mode", i == 0,
                     "ice-tcp", FALSE, "upnp", FALSE, NULL);
        if (!nice_agent_add_local_address(peer->agent, &local)) goto cleanup;

        g_signal_connect(peer->agent, "candidate-gathering-done",
                         G_CALLBACK(gathering_done), peer);
        g_signal_connect(peer->agent, "component-state-changed",
                         G_CALLBACK(state_changed), peer);

        peer->stream = nice_agent_add_stream(peer->agent, 1);
        if (peer->stream == 0) goto cleanup;
        if (!nice_agent_attach_recv(peer->agent, peer->stream, 1, NULL,
                                    receive_data, peer)) goto cleanup;
        if (!nice_agent_gather_candidates(peer->agent, peer->stream)) goto cleanup;
    }

    if (!test.failed && !(test.peers[0].gathered && test.peers[1].gathered))
        g_main_loop_run(test.loop);
    if (test.failed) goto cleanup;
    g_print("Candidatos locais coletados.\n");

    for (int i = 0; i < 2; i++) {
        peer_t *local_peer = &test.peers[i];
        peer_t *remote_peer = &test.peers[1 - i];
        gchar *ufrag = NULL;
        gchar *password = NULL;
        GSList *candidates;
        gboolean credentials_ok;
        int added;

        if (!nice_agent_get_local_credentials(local_peer->agent,
                local_peer->stream, &ufrag, &password)) goto cleanup;

        candidates = nice_agent_get_local_candidates(local_peer->agent,
                                                     local_peer->stream, 1);
        credentials_ok = nice_agent_set_remote_credentials(remote_peer->agent,
                            remote_peer->stream, ufrag, password);
        added = nice_agent_set_remote_candidates(remote_peer->agent,
                                                 remote_peer->stream, 1, candidates);
        g_free(ufrag);
        g_free(password);
        g_slist_free_full(candidates, (GDestroyNotify)nice_candidate_free);
        if (!credentials_ok || added <= 0) goto cleanup;
    }

    if (!test.failed && !(test.peers[0].ready && test.peers[1].ready))
        g_main_loop_run(test.loop);
    if (test.failed) goto cleanup;
    g_print("Ambos os peers em READY.\n");

    for (int i = 0; i < 2; i++) {
        peer_t *peer = &test.peers[i];
        guint length = strlen(peer->message);

        if (nice_agent_send(peer->agent, peer->stream, 1,
                            length, peer->message) != (int)length) goto cleanup;
    }

    if (!test.failed && !(test.peers[0].received && test.peers[1].received))
        g_main_loop_run(test.loop);
    if (!test.failed && test.peers[0].received && test.peers[1].received)
        result = EXIT_SUCCESS;

cleanup:
    g_source_remove(timeout);
    for (int i = 0; i < 2; i++) {
        if (test.peers[i].agent != NULL)
            g_object_unref(test.peers[i].agent);
    }
    g_main_loop_unref(test.loop);
    if (result != EXIT_SUCCESS) g_printerr("Teste local da libnice falhou.\n");
    return result;
}
