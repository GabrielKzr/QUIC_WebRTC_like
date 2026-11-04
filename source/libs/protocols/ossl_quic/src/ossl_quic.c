#include "ossl_quic.h"
#include <sys/select.h>

static const unsigned char alpn_osslquic_own[] = {
    0x08, 0x6f, 0x73, 0x73, 0x6c, 0x71, 0x75, 0x69, 0x63
};

static int ossl_quic_select_alpn(SSL *ssl, const unsigned char **out, unsigned char *out_len, const unsigned char *in, unsigned int in_len, void *arg);
static const char *suite_to_name(ossl_tls13_suite_id_t s);
static int ossl_quic_apply_tls13_ciphersuites(SSL_CTX *ctx, const ossl_quic_config_t *cfg);
static udp_conn_status_t ossl_quic_init(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_deinit(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_is_closed(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_get_timeout(const udp_conn_t* conn, struct timeval *tv);
static udp_conn_status_t ossl_quic_hole_punching(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_pre_connect(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_wait(const udp_conn_t* conn, SSL* ssl, int64_t deadline);
static udp_conn_status_t ossl_quic_connect(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_open_streams(const udp_conn_t* conn, int64_t deadline);
static udp_conn_status_t ossl_quic_read_control(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_udp_send_ka(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_disconnect(const udp_conn_t* conn);
static udp_conn_status_t ossl_quic_udp_send(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *sent_bytes);
static udp_conn_status_t ossl_quic_udp_recv(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *recv_bytes);

static int ossl_quic_select_alpn(SSL *ssl,
                       const unsigned char **out, unsigned char *out_len,
                       const unsigned char *in, unsigned int in_len,
                       void *arg)
{
    if (SSL_select_next_proto((unsigned char **)out, out_len,
                              alpn_osslquic_own, sizeof(alpn_osslquic_own),
                              in, in_len) != OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    return SSL_TLSEXT_ERR_OK;
}

static const char *suite_to_name(ossl_tls13_suite_id_t s)
{
    switch (s) {
    case OSSL_TLS13_SUITE_AES_128_GCM_SHA256:       return "TLS_AES_128_GCM_SHA256";
    case OSSL_TLS13_SUITE_AES_256_GCM_SHA384:       return "TLS_AES_256_GCM_SHA384";
    case OSSL_TLS13_SUITE_CHACHA20_POLY1305_SHA256: return "TLS_CHACHA20_POLY1305_SHA256";
    case OSSL_TLS13_SUITE_AES_128_CCM_SHA256:       return "TLS_AES_128_CCM_SHA256";
    case OSSL_TLS13_SUITE_AES_128_CCM_8_SHA256:     return "TLS_AES_128_CCM_8_SHA256";
    default: return NULL;
    }
}

static int ossl_quic_apply_tls13_ciphersuites(SSL_CTX *ctx, const ossl_quic_config_t *cfg)
{
    if (ctx == NULL || cfg == NULL) return 0;

    if (cfg->tls13_preset == OSSL_TLS13_PRESET_DEFAULT) {
        return 1; /* deixa OpenSSL usar default */
    }

    if (cfg->tls13_preset == OSSL_TLS13_PRESET_BALANCED) {
        return SSL_CTX_set_ciphersuites(
            ctx,
            "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256"
        );
    }

    if (cfg->tls13_preset == OSSL_TLS13_PRESET_PERF_AES128) {
        return SSL_CTX_set_ciphersuites(
            ctx,
            "TLS_AES_128_GCM_SHA256:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_256_GCM_SHA384"
        );
    }

    /* CUSTOM */
    if (cfg->tls13_suites_len == 0 || cfg->tls13_suites_len > OSSL_TLS13_MAX_SUITES) return 0;

    char buf[256];
    size_t off = 0;

    for (size_t i = 0; i < cfg->tls13_suites_len; i++) {
        const char *name = suite_to_name(cfg->tls13_suites[i]);
        size_t nlen;

        if (name == NULL) return 0;
        nlen = strlen(name);

        if (off + nlen + 1 >= sizeof(buf)) return 0;
        if (i > 0) buf[off++] = ':';
        memcpy(&buf[off], name, nlen);
        off += nlen;
    }

    buf[off] = '\0';
    return SSL_CTX_set_ciphersuites(ctx, buf);
}

static udp_conn_status_t ossl_quic_init(const udp_conn_t* conn) {

    if(conn == NULL)           return UDP_CONN_ERR;

    ossl_quic_data_t* data = conn->data;
    ossl_quic_config_t* config = conn->config;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    udp_session_t* udp_session = conn->udp_session;
     
    if(data == NULL)           return UDP_CONN_ERR;
    if(config == NULL)         return UDP_CONN_ERR;
    if(ctrl == NULL)           return UDP_CONN_ERR;
    if(udp_session == NULL)    return UDP_CONN_ERR;
    if(conn->udp_conn_callback == NULL) return UDP_CONN_ERR;
     
    if(ctrl->init) return UDP_CONN_ALREADY_INIT;

    if(ctrl->mode == 'c') {

        /*
            Configuring SSL context
        */
        if (data->ctx != NULL) {
            SSL_CTX_free(data->ctx);
            data->ctx = NULL;
        }

        data->ctx = SSL_CTX_new(OSSL_QUIC_client_method());
        if(data->ctx == NULL) {
            DEBUG_PRINT("[ERROR] Error while creating ssl context\n");
            return UDP_CONN_ERR;
        }

        SSL_CTX_set_verify(data->ctx, SSL_VERIFY_NONE, NULL);
        
        if(SSL_CTX_set_alpn_protos(data->ctx, alpn_osslquic_own, sizeof(alpn_osslquic_own))) {
            DEBUG_PRINT("[ERROR] Error trying to define alpn proto\n");
            return UDP_CONN_ERR;
        }

        /*
            Defining ciphersuites
        */
        if (!ossl_quic_apply_tls13_ciphersuites(data->ctx, config)) {
            DEBUG_PRINT("[ERROR] Error setting ciphersuites\n");
            return UDP_CONN_ERR;
        }

    } else if(ctrl->mode == 's') {

        if(config->cert_file == NULL || config->cert_file[0] == '\0' ||
           config->key_file == NULL || config->key_file[0] == '\0') {
            DEBUG_PRINT("[ERROR] Server certificate and key paths are required\n");
            return UDP_CONN_ERR;
        }

        /*
            Creating context
        */
        if (data->ctx != NULL) {
            SSL_CTX_free(data->ctx);
            data->ctx = NULL;
        }

        data->ctx = SSL_CTX_new(OSSL_QUIC_server_method());
        if(data->ctx == NULL) {
            DEBUG_PRINT("[ERROR] Error while creating ssl context\n");
            return UDP_CONN_ERR;
        }

        /*
            Loading certs for AUTH
        */
        if(SSL_CTX_use_certificate_chain_file(data->ctx, config->cert_file) <= 0) {
            DEBUG_PRINT("[ERROR] Error while trying to load cert file\n");
            return UDP_CONN_ERR;
        }
        
        if(SSL_CTX_use_PrivateKey_file(data->ctx, config->key_file, SSL_FILETYPE_PEM) <= 0) {
            DEBUG_PRINT("[ERROR] Error while trying to load key file\n");
            return UDP_CONN_ERR;
        }

        if (!SSL_CTX_check_private_key(data->ctx)) {
            DEBUG_PRINT("[ERROR] Private key does not match cert\n");
            return UDP_CONN_ERR;
        }            

        SSL_CTX_set_min_proto_version(data->ctx, TLS1_3_VERSION);

        /*
            Defining ciphersuites
        */
        if (!ossl_quic_apply_tls13_ciphersuites(data->ctx, config)) {
            DEBUG_PRINT("[ERROR] Error setting ciphersuites\n");
            return UDP_CONN_ERR;
        }

        /*
            Defining aplication layer protocol used (default for osslquic)        
        */
        SSL_CTX_set_alpn_select_cb(data->ctx, ossl_quic_select_alpn, NULL);
    } else {
        DEBUG_PRINT("[Error] Unknown mode\n");
        return UDP_CONN_ERR;
    }

    data->bio_method = ossl_quic_bio_method_new();
    if(data->bio_method == NULL) {
        SSL_CTX_free(data->ctx);
        data->ctx = NULL;
        return UDP_CONN_ERR;
    }

    ctrl->init = 1;
    data->closed = 0;
    data->close_status = UDP_CONN_OK;

    DEBUG_PRINT("[DEBUG] ossl_quic_init()\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_deinit(const udp_conn_t* conn) {

    udp_session_t* udp_session = conn->udp_session;
    ossl_quic_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    // Um erro de fechamento nao pode impedir a liberacao dos recursos.
    if (!data->closed)
        ossl_quic_disconnect(conn);

    SSL_free(data->control);
    data->control = NULL;
    SSL_free(data->stream);
    data->stream = NULL;
    SSL_free(data->conn);
    data->conn = NULL;
    SSL_free(data->listener);
    data->listener = NULL;
    BIO_meth_free(data->bio_method);
    data->bio_method = NULL;
    SSL_CTX_free(data->ctx);
    data->ctx = NULL;

    ctrl->init = 0;
    DEBUG_PRINT("[DEBUG] ossl_quic_deinit()\n");
    return data->close_status;
}
 
static udp_conn_status_t ossl_quic_is_closed(const udp_conn_t* conn) {
    ossl_quic_data_t* data = conn->data;
    if(data == NULL) return UDP_CONN_ERR;
    return data->closed ? UDP_CONN_CLOSED : UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_get_timeout(const udp_conn_t* conn, struct timeval *tv) {
    ossl_quic_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    int is_infinite = 0;

    if (ctrl == NULL || data == NULL || data->conn == NULL || tv == NULL)
        return UDP_CONN_ERR;
    if (!ctrl->init) return UDP_CONN_NOT_INIT;

    tv->tv_sec = 0;
    tv->tv_usec = 0;

    if (!SSL_get_event_timeout(data->conn, tv, &is_infinite)) return UDP_CONN_ERR;
    
    if (is_infinite || tv->tv_sec >= 5) {
        tv->tv_sec = 5;
        tv->tv_usec = 0;
    }

    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_hole_punching(const udp_conn_t* conn) {
    uint8_t hole_punched = 0;
    const char msg[] = "01\n";
    char buffer[4] = {0};
    size_t nread = 0;
    struct timeval no_wait = {0};

    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if (conn->transport == NULL || ctrl == NULL) return UDP_CONN_ERR;
    if (!conn->transport->opened) return UDP_CONN_NOT_INIT;
    if (ctrl->mode != 'c' && ctrl->mode != 's') return UDP_CONN_UNKNOWN_MODE;

    /* IDLE: timeout de 1s para fase de descoberta */
    struct timeval udp_recv_timeout = {
        .tv_sec = 1,
        .tv_usec = 0
    };

    DEBUG_PRINT("[DEBUG] Trying a connection with remote end\n");

    /* Only the client initiates discovery; the server waits for its trigger. */
    while (1) {

        DEBUG_PRINT("[DEBUG] Attempting to connect\n");
        
        if (ctrl->mode == 'c' &&
            udp_transport_send(conn->transport, msg, sizeof(msg) - 1, &no_wait) != UDP_TRANSPORT_OK) {
            DEBUG_PRINT("[ERROR] Hole punching send failed\n");
            return UDP_CONN_ERR;
        }

        if (ctrl->mode == 's') {
            if (udp_transport_send(conn->transport, "\0", 1, &no_wait) != UDP_TRANSPORT_OK) {
                DEBUG_PRINT("[ERROR] Hole punching keep-alive failed\n");
                return UDP_CONN_ERR;
            }
        }

        if (udp_transport_recv(conn->transport, buffer, 3, &nread, &udp_recv_timeout) != UDP_TRANSPORT_OK) {
            DEBUG_PRINT("[DEBUG] No discovery packet received\n");
            continue;
        }

        if (nread == 3 && memcmp(buffer, msg, 3) == 0) {
            DEBUG_PRINT("[REMOTE] Discovered a remote end\n");
            break;
        }

        /* Keep-alives recebidos durante a descoberta são ignorados. */
    }

    /* TRANSIENT: timeout curto + recv não bloqueante */
    for (int i = 0; i < 10; i++) {
        if (udp_transport_send(conn->transport, msg, sizeof(msg) - 1, &no_wait) != UDP_TRANSPORT_OK) {
            DEBUG_PRINT("[ERROR] Transient send failed\n");
            return UDP_CONN_ERR;
        }

        msleep(50);

        if (!hole_punched) {
            if (udp_transport_recv(conn->transport, buffer, 3, &nread, &no_wait) == UDP_TRANSPORT_OK) {
                if (nread == 3 && memcmp(buffer, msg, 3) == 0) {
                    hole_punched = 1;
                }
            }
        }
    }

    if (!hole_punched) {
        DEBUG_PRINT("[WARNING] Hole punching did not occured\n");
        return UDP_CONN_ERR;
    }

    DEBUG_PRINT("[REMOTE] Connection opened to remote end\n");
    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_pre_connect(const udp_conn_t* conn) {

    udp_session_t* udp_session = conn->udp_session;
    ossl_quic_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    SSL* ssl;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(ctrl->mode == 'c') {

        data->conn = SSL_new(data->ctx);
        if(data->conn == NULL) {
            DEBUG_PRINT("[ERROR] Error trying to create connection\n");
            return UDP_CONN_ERR;
        }

        if (!SSL_set_default_stream_mode(data->conn, SSL_DEFAULT_STREAM_MODE_NONE) ||
            !SSL_set_incoming_stream_policy(data->conn, SSL_INCOMING_STREAM_POLICY_ACCEPT, 0))
            return UDP_CONN_ERR;

        SSL_set_connect_state(data->conn);
        ssl = data->conn;

    } else if(ctrl->mode == 's') {

        data->conn = NULL;
        data->listener = SSL_new_listener(data->ctx, 0);
        if(data->listener == NULL) {
            DEBUG_PRINT("[ERROR] Error trying to create listener\n");
            return UDP_CONN_ERR;
        }

        ssl = data->listener;

    } else {
        DEBUG_PRINT("[DEBUG] Unknown mode\n");
        return UDP_CONN_ERR;
    }

    data->bio_config = (ossl_quic_bio_config_t){
        .transport = conn->transport,
        .peer = udp_session->dst,
        .max_datagram_size = 1200
    };

    BIO* bio = ossl_quic_bio_new(data->bio_method, &data->bio_config);
    if(bio == NULL) return UDP_CONN_ERR;
    if(!BIO_up_ref(bio)) {
        BIO_free(bio);
        return UDP_CONN_ERR;
    }

    SSL_set0_rbio(ssl, bio);
    SSL_set0_wbio(ssl, bio);

    if(!SSL_set_blocking_mode(ssl, 0)) {
        DEBUG_PRINT("[ERROR] Error setting non-blocking mode\n");
        return UDP_CONN_ERR;
    }

    if(ctrl->mode == 'c') {
        BIO_ADDR* peer = BIO_ADDR_new();
        if(peer == NULL) return UDP_CONN_ERR;
        int configured = BIO_ADDR_rawmake(peer, udp_session->dst.sin_family,
                                         &udp_session->dst.sin_addr, sizeof(udp_session->dst.sin_addr),
                                         udp_session->dst.sin_port) &&
                         SSL_set1_initial_peer_addr(ssl, peer);
        BIO_ADDR_free(peer);
        if(!configured) return UDP_CONN_ERR;
    }

    if(ctrl->mode == 's' && !SSL_listen(ssl)) {
        DEBUG_PRINT("[ERROR] Error trying to listen from ssl_listen()\n");
        return UDP_CONN_ERR;
    }

    DEBUG_PRINT("[DEBUG] UDP transport BIO attached to OpenSSL QUIC\n");

    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_connect(const udp_conn_t* conn) {

    ossl_quic_data_t* data = conn->data;
    udp_session_t* udp_session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(data == NULL)        return UDP_CONN_ERR;
    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    udp_conn_status_t status = ossl_quic_pre_connect(conn);
    if(status != UDP_CONN_OK) return status;

    int64_t started = monotonic_ms();
    if(started < 0) return UDP_CONN_ERR;
    const int64_t deadline = started + 5000;

    if(ctrl->mode == 'c') {

        while(1) {
            ERR_clear_error();
            int result = SSL_connect(data->conn);
            if(result == 1) break;

            int error = SSL_get_error(data->conn, result);
            if(error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
                ERR_print_errors_fp(stderr);
                return UDP_CONN_ERR;
            }

            if(ossl_quic_wait(conn, data->conn, deadline) != UDP_CONN_OK)
                return UDP_CONN_ERR;
        }

        DEBUG_PRINT("[DEBUG] Connected to QUIC server\n");

    } else if(ctrl->mode == 's') {

        DEBUG_PRINT("[DEBUG] QUIC server listening\n");

        while(data->conn == NULL) {
            ERR_clear_error();
            data->conn = SSL_accept_connection(data->listener, SSL_ACCEPT_CONNECTION_NO_BLOCK);
            if(data->conn != NULL) break;

            if(ERR_peek_error() != 0) {
                ERR_print_errors_fp(stderr);
                return UDP_CONN_ERR;
            }

            if(ossl_quic_wait(conn, data->listener, deadline) != UDP_CONN_OK)
                return UDP_CONN_ERR;
        }

        if(!SSL_set_blocking_mode(data->conn, 0) ||
           !SSL_set_default_stream_mode(data->conn, SSL_DEFAULT_STREAM_MODE_NONE) ||
           !SSL_set_incoming_stream_policy(data->conn, SSL_INCOMING_STREAM_POLICY_ACCEPT, 0))
            return UDP_CONN_ERR;

        while(!SSL_is_init_finished(data->conn)) {
            ERR_clear_error();
            int result = SSL_do_handshake(data->conn);
            if(result == 1) break;

            int error = SSL_get_error(data->conn, result);
            if(error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
                ERR_print_errors_fp(stderr);
                return UDP_CONN_ERR;
            }

            if(ossl_quic_wait(conn, data->conn, deadline) != UDP_CONN_OK)
                return UDP_CONN_ERR;
        }

        DEBUG_PRINT("[DEBUG] Connected to QUIC client\n");

    } else {
        DEBUG_PRINT("[ERROR] Unknown mode\n");
        return UDP_CONN_ERR;
    }

    status = ossl_quic_open_streams(conn, deadline);
    if (status != UDP_CONN_OK) return status;

    conn->udp_conn_callback(conn, OSSL_QUIC_UDP_CONNECTED, NULL, 0);

    data->closed = 0;

    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_open_streams(const udp_conn_t* conn, int64_t deadline) {
    ossl_quic_data_t* data = conn->data;

    if (conn->ctrl->mode == 'c') {
        SSL** streams[] = {&data->stream, &data->control};
        for (int i = 0; i < 2; i++) {
            while (*streams[i] == NULL) {
                if (!SSL_handle_events(data->conn)) return UDP_CONN_ERR;
                ERR_clear_error();
                *streams[i] = SSL_new_stream(data->conn, SSL_STREAM_FLAG_NO_BLOCK);
                if (*streams[i] != NULL) break;
                if (ERR_peek_error() != 0 ||
                    ossl_quic_wait(conn, data->conn, deadline) != UDP_CONN_OK)
                    return UDP_CONN_ERR;
            }
        }
    } else {
        while (data->stream == NULL || data->control == NULL) {
            if (!SSL_handle_events(data->conn)) return UDP_CONN_ERR;
            ERR_clear_error();
            SSL* incoming = SSL_accept_stream(data->conn, SSL_ACCEPT_STREAM_NO_BLOCK);
            if (incoming == NULL) {
                if (ERR_peek_error() != 0 ||
                    ossl_quic_wait(conn, data->conn, deadline) != UDP_CONN_OK)
                    return UDP_CONN_ERR;
                continue;
            }

            uint64_t id = SSL_get_stream_id(incoming);
            if (SSL_get_stream_type(incoming) != SSL_STREAM_TYPE_BIDI ||
                (id != 0 && id != 4) ||
                (id == 0 && data->stream != NULL) ||
                (id == 4 && data->control != NULL)) {
                SSL_free(incoming);
                return UDP_CONN_ERR;
            }
            if (id == 0) data->stream = incoming;
            else data->control = incoming;
        }
    }

    SSL* streams[] = {data->stream, data->control};
    const unsigned char markers[] = {'D', 'C'};

    /* Anunciar ambos antes de esperar a confirmação evita dependência de dados TCP. */
    for (int reading = 0; reading < 2; reading++) {
        for (int i = 0; i < 2; i++) {
            while (1) {
                unsigned char marker = markers[i];
                size_t transferred = 0;
                ERR_clear_error();
                int result = reading
                    ? SSL_read_ex(streams[i], &marker, 1, &transferred)
                    : SSL_write_ex(streams[i], &marker, 1, &transferred);
                if (result == 1) {
                    if (transferred != 1 || marker != markers[i]) {
                        fprintf(stderr, "QUIC stream opening marker mismatch: phase=%d stream=%d count=%zu marker=%u\n", reading, i, transferred, marker);
                        return UDP_CONN_ERR;
                    }
                    break;
                }
                int error = SSL_get_error(streams[i], result);
                if (error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
                    fprintf(stderr, "QUIC stream opening failed: phase=%d stream=%d SSL error=%d\n", reading, i, error);
                    ERR_print_errors_fp(stderr);
                    return UDP_CONN_ERR;
                }
                if (ossl_quic_wait(conn, data->conn, deadline) != UDP_CONN_OK)
                    return UDP_CONN_ERR;
            }
        }
    }

    DEBUG_PRINT("[DEBUG] QUIC data stream 0 and control stream 4 established\n");
    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_read_control(const udp_conn_t* conn) {
    ossl_quic_data_t* data = conn->data;
    unsigned char buffer[64];

    if (data->control == NULL) return UDP_CONN_ERR;

    while (1) {
        size_t received = 0;
        ERR_clear_error();
        int result = SSL_read_ex(data->control, buffer, sizeof(buffer), &received);
        if (result == 1) {
            for (size_t i = 0; i < received; i++) {
                if (buffer[i] != 0) return UDP_CONN_ERR;
            }
            continue;
        }
        int error = SSL_get_error(data->control, result);
        if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
            return UDP_CONN_OK;
        return UDP_CONN_ERR;
    }
}

static udp_conn_status_t ossl_quic_udp_send_ka(const udp_conn_t* conn) {

    udp_session_t* udp_session = conn->udp_session;
    ossl_quic_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;
    
    if(data->control == NULL) return UDP_CONN_NOT_INIT;

    size_t written = 0;
    ERR_clear_error();
    int result = SSL_write_ex(data->control, "\0", 1, &written);
    if (result != 1) {
        int error = SSL_get_error(data->control, result);
        if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
            return UDP_CONN_OK;
        return UDP_CONN_ERR;
    }

    return written == 1 ? UDP_CONN_OK : UDP_CONN_ERR;
}

static int ossl_quic_record_close_status(ossl_quic_data_t* data)
{
    SSL_CONN_CLOSE_INFO info = {0};
    if (!SSL_get_conn_close_info(data->conn, &info, sizeof(info)))
        return 0;

    if (info.error_code != 0 && data->close_status == UDP_CONN_OK) {
        fprintf(stderr, "QUIC %s close error: %llu (flags=%u)\n",
                (info.flags & SSL_CONN_CLOSE_FLAG_LOCAL) ? "local" : "remote",
                (unsigned long long)info.error_code, info.flags);
        data->close_status = UDP_CONN_ERR;
    }
    return 1;
}

static udp_conn_status_t ossl_quic_wait(const udp_conn_t* conn, SSL* ssl, int64_t deadline)
{
    int64_t now = monotonic_ms();
    if (now < 0 || now >= deadline)
        return UDP_CONN_ERR;

    struct timeval timeout = {0};
    int infinite;
    if (!SSL_get_event_timeout(ssl, &timeout, &infinite))
        return UDP_CONN_ERR;

    int64_t remaining_us = (deadline - now) * 1000;
    if (infinite ||
        remaining_us < (int64_t)timeout.tv_sec * 1000000 + timeout.tv_usec) {
        timeout.tv_sec = remaining_us / 1000000;
        timeout.tv_usec = remaining_us % 1000000;
    }

    unsigned requested_events = 0;
    unsigned events = 0;

    if (SSL_net_read_desired(ssl))
        requested_events |= UDP_TRANSPORT_READABLE;
    if (SSL_net_write_desired(ssl))
        requested_events |= UDP_TRANSPORT_WRITABLE;

    if (udp_transport_wait(conn->transport, requested_events, &timeout, &events) != UDP_TRANSPORT_OK)
        return UDP_CONN_ERR;

    now = monotonic_ms();
    if (now < 0 || now >= deadline)
        return UDP_CONN_ERR;

    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_finish_shutdown(const udp_conn_t* conn)
{
    ossl_quic_data_t* data = conn->data;
    int64_t started = monotonic_ms();
    if (started < 0)
        return UDP_CONN_ERR;
    const int64_t deadline = started + 5000;

    while(1) {
        
        // Handle any events that are pending on the connection before try to shutdown
        if (!SSL_handle_events(data->conn))
            return UDP_CONN_ERR;
        
        // try to shutdown, if ok, return, if not, check error, if no error then wait for events
        int result = SSL_shutdown(data->conn);
        if (result == 1) {
            ossl_quic_record_close_status(data);
            return data->close_status;
        }
        if (result < 0) {
            int error = SSL_get_error(data->conn, result);
            if (error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
                fprintf(stderr, "QUIC shutdown failed: SSL error %d\n", error);
                ERR_print_errors_fp(stderr);
                return UDP_CONN_ERR;
            }
        }

        ossl_quic_record_close_status(data);

        if (ossl_quic_wait(conn, data->conn, deadline) != UDP_CONN_OK) {
            fprintf(stderr, "QUIC shutdown wait failed or exceeded 5 seconds\n");
            return UDP_CONN_ERR;
        }
    }
}

static udp_conn_status_t ossl_quic_disconnect(const udp_conn_t* conn)
{
    ossl_quic_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    if (data == NULL || ctrl == NULL || conn->udp_session == NULL)
        return UDP_CONN_ERR;
    if (!ctrl->init)
        return UDP_CONN_NOT_INIT;
    if (data->closed)
        return data->close_status;

    udp_conn_status_t status = data->conn != NULL
        ? ossl_quic_finish_shutdown(conn) : UDP_CONN_ERR;
    if (status < 0)
        data->close_status = status;

    // Estado terminal mesmo em erro/timeout; deinit ainda libera tudo.
    data->closed = 1;
    conn->udp_conn_callback(conn, OSSL_QUIC_UDP_DISCONNECTED, NULL, 0);
    return data->close_status;
}

static udp_conn_status_t ossl_quic_udp_send(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *sent_bytes) {

    udp_session_t* udp_session = conn->udp_session;
    ossl_quic_data_t* data = conn->data;
    udp_conn_ctrl_t* ctrl = conn->ctrl;
    size_t sent = 0;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;
    if(data->closed) return UDP_CONN_CLOSED;
    if(data->stream == NULL) return UDP_CONN_NOT_INIT;

    DEBUG_PRINT("[DEBUG] Sending %zu bytes through QUIC\n", nbytes);

    if (!SSL_write_ex(data->stream, buf, nbytes, &sent)) {
        ERR_print_errors_fp(stderr);
        return UDP_CONN_ERR;
    }

    DEBUG_PRINT("[DEBUG] Sent %zu bytes through QUIC\n", sent);

    if (sent_bytes != NULL) {
        *sent_bytes = sent;
    }
    conn->udp_conn_callback(conn, OSSL_QUIC_UDP_DATA_SENT, buf, sent);

    return UDP_CONN_OK;
}

static udp_conn_status_t ossl_quic_udp_recv(const udp_conn_t* conn, void* buf, size_t nbytes, size_t *recv_bytes) {
    ossl_quic_data_t *data = conn->data;
    
    if (recv_bytes == NULL) return UDP_CONN_ERR;
    
    *recv_bytes = 0;

    if (data == NULL || conn->ctrl == NULL || buf == NULL || nbytes == 0) return UDP_CONN_ERR;
    if (!conn->ctrl->init) return UDP_CONN_NOT_INIT;
    if (data->closed) return UDP_CONN_CLOSED;

    int ret = SSL_read_ex(data->stream, buf, nbytes, recv_bytes);
    if (ret == 1) {
        conn->udp_conn_callback(conn, OSSL_QUIC_UDP_RECV_DATA, buf, *recv_bytes);
        return UDP_CONN_OK;
    }

    int err = SSL_get_error(data->stream, ret);
    *recv_bytes = 0;

    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
        return UDP_CONN_OK;
    return UDP_CONN_ERR;
}

static size_t ossl_quic_pending_data(const udp_conn_t *conn) {
    ossl_quic_data_t *data = conn->data;
    return data->stream != NULL ? (size_t)SSL_pending(data->stream) : 0;
}

static udp_conn_status_t ossl_quic_process_events(const udp_conn_t *conn) {

    ossl_quic_data_t *data = conn->data;

    if (data == NULL || data->conn == NULL) return UDP_CONN_ERR;
    if (!SSL_handle_events(data->conn)) return UDP_CONN_ERR;
    if (ossl_quic_record_close_status(data))
        return ossl_quic_disconnect(conn);

    return ossl_quic_read_control(conn);
}

udp_conn_generic_api_t ossl_quic_api = {
    .init = ossl_quic_init,
    .deinit = ossl_quic_deinit,
    .is_closed = ossl_quic_is_closed,
    .get_timeout = ossl_quic_get_timeout,
    .process_events = ossl_quic_process_events,
    .hole_punching = ossl_quic_hole_punching,
    .connect = ossl_quic_connect,
    .udp_send = ossl_quic_udp_send,
    .udp_recv = ossl_quic_udp_recv,
    .pending_data = ossl_quic_pending_data,
    .udp_send_ka = ossl_quic_udp_send_ka,
    .disconnect = ossl_quic_disconnect,
};
