#include "ice_udp_transport.h"
#include <string.h>

static udp_transport_status_t ice_wait(udp_transport_t *transport, unsigned requested_events, const struct timeval *timeout, unsigned *events);

static void ice_gathering_done(NiceAgent *agent, guint stream_id, gpointer user_data) {
    udp_transport_t *transport = user_data;
    const ice_udp_transport_config_t *config = transport->config;
    ice_udp_transport_data_t *ice = transport->data;

    if (stream_id != ice->stream_id) return;
    ice->gathering_done = TRUE;

    char *ufrag = NULL, *password = NULL;
    char *remote_ufrag = NULL, *remote_password = NULL;
    GSList *candidates = NULL, *remote_candidates = NULL;
    gboolean success = FALSE;

    if (!nice_agent_get_local_credentials(agent, stream_id, &ufrag, &password)) {
        goto cleanup;
    }

    candidates = nice_agent_get_local_candidates(agent, stream_id, 1);
    if (candidates == NULL) goto cleanup;

    if (config->signaling_cb(
            transport, ufrag, password, candidates,
            &remote_ufrag, &remote_password,
            &remote_candidates) != UDP_TRANSPORT_OK) {
        goto cleanup;
    }

    if (remote_ufrag == NULL || remote_password == NULL ||
        remote_candidates == NULL) {
        goto cleanup;
    }

    if (!nice_agent_set_remote_credentials(
            agent, stream_id, remote_ufrag, remote_password)) {
        goto cleanup;
    }

    if (nice_agent_set_remote_candidates(agent, stream_id, 1, remote_candidates) <= 0) {
        goto cleanup;
    }

    success = TRUE;

cleanup:
    g_free(ufrag);
    g_free(password);
    g_free(remote_ufrag);
    g_free(remote_password);
    g_slist_free_full(candidates, (GDestroyNotify)nice_candidate_free);
    g_slist_free_full(remote_candidates, (GDestroyNotify)nice_candidate_free);

    if (!success) ice->state = NICE_COMPONENT_STATE_FAILED;
}

static void ice_state_changed(NiceAgent *agent, guint stream_id, guint component_id, guint state, gpointer user_data) {
    ice_udp_transport_data_t *ice = user_data;
    (void)agent;

    if (stream_id == ice->stream_id && component_id == 1) {
        ice->state = (NiceComponentState)state;
    }
}

static udp_transport_status_t ice_open(udp_transport_t *transport) {
    const ice_udp_transport_config_t *config = transport->config;
    ice_udp_transport_data_t *ice = transport->data;

    if (config == NULL || ice == NULL || config->signaling_cb == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (config->stun_server != NULL &&
        (config->stun_port == 0 || config->stun_port > 65535)) {
        return UDP_TRANSPORT_ERROR;
    }

    if (config->turn_server != NULL &&
        (config->turn_port == 0 || config->turn_port > 65535 ||
         config->turn_username == NULL || config->turn_password == NULL)) {
        return UDP_TRANSPORT_ERROR;
    }

    *ice = (ice_udp_transport_data_t){0};
    ice->context = g_main_context_new();
    ice->agent = nice_agent_new(ice->context, NICE_COMPATIBILITY_RFC5245);
    if (ice->agent == NULL) goto fail;

    g_object_set(ice->agent,
                 "controlling-mode", config->controlling,
                 "ice-tcp", FALSE,
                 "upnp", FALSE,
                 NULL);

    if (config->stun_server != NULL) {
        g_object_set(ice->agent,
                     "stun-server", config->stun_server,
                     "stun-server-port", config->stun_port,
                     NULL);
    }

    if (config->local_address != NULL) {
        NiceAddress local;
        nice_address_init(&local);

        if (!nice_address_set_from_string(&local, config->local_address) ||
            !nice_agent_add_local_address(ice->agent, &local)) {
            goto fail;
        }
    }

    ice->stream_id = nice_agent_add_stream(ice->agent, 1);
    if (ice->stream_id == 0) goto fail;

    if (config->turn_server != NULL &&
        !nice_agent_set_relay_info(
            ice->agent, ice->stream_id, 1,
            config->turn_server, config->turn_port,
            config->turn_username, config->turn_password,
            NICE_RELAY_TYPE_TURN_UDP)) {
        goto fail;
    }

    g_signal_connect(ice->agent, "candidate-gathering-done",
                     G_CALLBACK(ice_gathering_done), transport);
    g_signal_connect(ice->agent, "component-state-changed",
                     G_CALLBACK(ice_state_changed), ice);

    if (!nice_agent_gather_candidates(ice->agent, ice->stream_id)) {
        goto fail;
    }

    return UDP_TRANSPORT_OK;

fail:
    if (ice->agent != NULL) g_object_unref(ice->agent);
    g_main_context_unref(ice->context);
    *ice = (ice_udp_transport_data_t){0};

    return UDP_TRANSPORT_ERROR;
}

static udp_transport_status_t ice_send(udp_transport_t *transport, const void *buffer, size_t length, const struct timeval *timeout) {
    ice_udp_transport_data_t *ice = transport->data;
    unsigned events = 0;

    udp_transport_status_t status = ice_wait(
        transport, UDP_TRANSPORT_WRITABLE, timeout, &events
    );
    if (status != UDP_TRANSPORT_OK) return status;
    if (!(events & UDP_TRANSPORT_WRITABLE)) return UDP_TRANSPORT_NOT_READY;

    GOutputVector vector = {.buffer = buffer, .size = length};
    NiceOutputMessage message = {.buffers = &vector, .n_buffers = 1};
    GError *error = NULL;

    gint count = nice_agent_send_messages_nonblocking(
        ice->agent, ice->stream_id, 1, &message, 1, NULL, &error
    );

    gboolean not_ready = count == 0 ||
        g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error(&error);

    if (count == 1) return UDP_TRANSPORT_OK;
    return not_ready ? UDP_TRANSPORT_NOT_READY : UDP_TRANSPORT_ERROR;
}

static udp_transport_status_t ice_recv(udp_transport_t *transport, void *buffer, size_t capacity, size_t *received, const struct timeval *timeout) {
    ice_udp_transport_data_t *ice = transport->data;
    unsigned events = 0;
    *received = 0;

    udp_transport_status_t status = ice_wait(
        transport, UDP_TRANSPORT_READABLE, timeout, &events
    );
    if (status != UDP_TRANSPORT_OK) return status;
    if (!(events & UDP_TRANSPORT_READABLE)) return UDP_TRANSPORT_NOT_READY;

    size_t length = ice->pending_size;
    ice->has_pending = FALSE;
    ice->pending_size = 0;

    if (length > capacity) return UDP_TRANSPORT_ERROR;
    if (length > 0) memcpy(buffer, ice->pending_packet, length);
    *received = length;

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t ice_get_timeout(udp_transport_t *transport, struct timeval *timeout, int *has_timeout) {
    ice_udp_transport_data_t *ice = transport->data;

    *timeout = (struct timeval){0};
    *has_timeout = 0;

    if (ice == NULL || ice->context == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    if (!g_main_context_acquire(ice->context)) {
        return UDP_TRANSPORT_ERROR;
    }

    gint priority;
    gint timeout_ms = -1;
    gboolean ready = g_main_context_prepare(ice->context, &priority);

    g_main_context_query(ice->context, priority, &timeout_ms, NULL, 0);
    g_main_context_release(ice->context);

    if (ready) timeout_ms = 0;

    if (timeout_ms >= 0) {
        *has_timeout = 1;
        timeout->tv_sec = timeout_ms / 1000;
        timeout->tv_usec = (timeout_ms % 1000) * 1000;
    }

    return UDP_TRANSPORT_OK;
}

static udp_transport_status_t ice_read_pending(ice_udp_transport_data_t *ice) {
    if (ice->has_pending) return UDP_TRANSPORT_OK;

    GInputVector vector = {
        .buffer = ice->pending_packet, .size = sizeof(ice->pending_packet)
    };
    NiceInputMessage message = {.buffers = &vector, .n_buffers = 1};
    GError *error = NULL;
    gint count = nice_agent_recv_messages_nonblocking(
        ice->agent, ice->stream_id, 1, &message, 1, NULL, &error
    );

    gboolean not_ready = count == 0 ||
        g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error(&error);

    if (count == 1) {
        ice->pending_size = message.length;
        ice->has_pending = TRUE;
        return UDP_TRANSPORT_OK;
    }

    return not_ready ? UDP_TRANSPORT_NOT_READY : UDP_TRANSPORT_ERROR;
}

static udp_transport_status_t ice_wait(udp_transport_t *transport, unsigned requested_events, const struct timeval *timeout, unsigned *events) {
    ice_udp_transport_data_t *ice = transport->data;
    *events = 0;

    if (ice == NULL || ice->agent == NULL || ice->context == NULL ||
        ice->state == NICE_COMPONENT_STATE_FAILED) {
        return UDP_TRANSPORT_ERROR;
    }

    if (ice_read_pending(ice) == UDP_TRANSPORT_ERROR ||
        ice->state == NICE_COMPONENT_STATE_FAILED) {
        return UDP_TRANSPORT_ERROR;
    }

    if (!g_main_context_acquire(ice->context)) {
        return UDP_TRANSPORT_ERROR;
    }

    udp_transport_status_t status = UDP_TRANSPORT_ERROR;
    GPtrArray *sockets = NULL;
    GArray *fds = g_array_new(FALSE, TRUE, sizeof(GPollFD));
    gint priority;
    gint timeout_ms = -1;
    gint glib_count;
    gboolean ready = g_main_context_prepare(ice->context, &priority);

    for (;;) {
        glib_count = g_main_context_query(
            ice->context, priority, &timeout_ms,
            (GPollFD *)fds->data, (gint)fds->len
        );
        if (glib_count <= (gint)fds->len) break;
        g_array_set_size(fds, glib_count);
    }
    g_array_set_size(fds, glib_count);

    sockets = nice_agent_get_sockets(ice->agent, ice->stream_id, 1);
    if (sockets == NULL) goto cleanup;

    for (guint i = 0; i < sockets->len; i++) {
        GSocket *socket = g_ptr_array_index(sockets, i);
        GPollFD fd = {.fd = -1};

        if (!g_socket_is_closed(socket)) {
            fd.fd = g_socket_get_fd(socket);
            if (!ice->has_pending) fd.events |= G_IO_IN;
            if ((requested_events & UDP_TRANSPORT_WRITABLE) &&
                ice->state == NICE_COMPONENT_STATE_READY) {
                fd.events |= G_IO_OUT;
            }
        }
        g_array_append_val(fds, fd);
    }

    gint tcp_index = -1;
    if (transport->tcp_fd >= 0 &&
        (requested_events & (TCP_TRANSPORT_READABLE | TCP_TRANSPORT_WRITABLE))) {
        GPollFD tcp = {.fd = transport->tcp_fd};
        if (requested_events & TCP_TRANSPORT_READABLE) tcp.events |= G_IO_IN;
        if (requested_events & TCP_TRANSPORT_WRITABLE) tcp.events |= G_IO_OUT;
        tcp_index = (gint)fds->len;
        g_array_append_val(fds, tcp);
    }

    if (timeout != NULL) {
        gint caller_ms = timeout->tv_sec > G_MAXINT / 1000
            ? G_MAXINT
            : (gint)MIN((gint64)timeout->tv_sec * 1000 +
                        (timeout->tv_usec + 999) / 1000, G_MAXINT);
        if (timeout_ms < 0 || caller_ms < timeout_ms) timeout_ms = caller_ms;
    }
    if (ready || (ice->has_pending && (requested_events & UDP_TRANSPORT_READABLE))) {
        timeout_ms = 0;
    }

    GPollFD *items = (GPollFD *)fds->data;
    if (g_poll(items, fds->len, timeout_ms) < 0) goto cleanup;

    if (g_main_context_check(ice->context, priority, items, glib_count)) {
        g_main_context_dispatch(ice->context);
    }

    if (ice->state == NICE_COMPONENT_STATE_FAILED ||
        ice_read_pending(ice) == UDP_TRANSPORT_ERROR ||
        ice->state == NICE_COMPONENT_STATE_FAILED) {
        goto cleanup;
    }

    if (ice->has_pending && (requested_events & UDP_TRANSPORT_READABLE)) {
        *events |= UDP_TRANSPORT_READABLE;
    }

    for (guint i = 0; i < sockets->len; i++) {
        gushort returned = items[glib_count + i].revents;
        if (returned & G_IO_NVAL) goto cleanup;
        if ((returned & G_IO_OUT) && (requested_events & UDP_TRANSPORT_WRITABLE) &&
            ice->state == NICE_COMPONENT_STATE_READY) {
            *events |= UDP_TRANSPORT_WRITABLE;
        }
    }

    if (tcp_index >= 0) {
        gushort returned = items[tcp_index].revents;
        if (returned & G_IO_NVAL) goto cleanup;
        if ((requested_events & TCP_TRANSPORT_READABLE) &&
            (returned & (G_IO_IN | G_IO_HUP | G_IO_ERR))) {
            *events |= TCP_TRANSPORT_READABLE;
        }
        if ((requested_events & TCP_TRANSPORT_WRITABLE) &&
            (returned & (G_IO_OUT | G_IO_HUP | G_IO_ERR))) {
            *events |= TCP_TRANSPORT_WRITABLE;
        }
    }

    status = UDP_TRANSPORT_OK;

cleanup:
    if (sockets != NULL) g_ptr_array_unref(sockets);
    g_array_unref(fds);
    g_main_context_release(ice->context);
    if (status != UDP_TRANSPORT_OK) *events = 0;
    return status;
}

static void ice_closed(GObject *source, GAsyncResult *result, gpointer user_data) {
    gboolean *closed = user_data;
    (void)source;
    (void)result;

    *closed = TRUE;
}

static udp_transport_status_t ice_close(udp_transport_t *transport) {
    ice_udp_transport_data_t *ice = transport->data;
    gboolean closed = FALSE;

    if (ice == NULL || ice->agent == NULL || ice->context == NULL) {
        return UDP_TRANSPORT_ERROR;
    }

    g_signal_handlers_disconnect_by_func(
        ice->agent, ice_gathering_done, transport
    );
    g_signal_handlers_disconnect_by_func(
        ice->agent, ice_state_changed, ice
    );

    g_main_context_push_thread_default(ice->context);
    nice_agent_close_async(ice->agent, ice_closed, &closed);

    while (!closed) {
        g_main_context_iteration(ice->context, TRUE);
    }

    g_main_context_pop_thread_default(ice->context);

    g_object_unref(ice->agent);
    g_main_context_unref(ice->context);
    *ice = (ice_udp_transport_data_t){0};

    return UDP_TRANSPORT_OK;
}

const udp_transport_ops_t ice_ops = {
    .open = ice_open,
    .send = ice_send,
    .recv = ice_recv,
    .get_timeout = ice_get_timeout,
    .wait = ice_wait,
    .close = ice_close
};
