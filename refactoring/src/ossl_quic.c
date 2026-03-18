#include "ossl_quic.h"

static const unsigned char alpn_osslquic_own[] = {
    0x08, 0x6f, 0x73, 0x73, 0x6c, 0x71, 0x75, 0x69, 0x63
};

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

int ossl_quic_apply_tls13_ciphersuites(SSL_CTX *ctx, const struct ossl_quic_config_t *cfg)
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

static int ossl_quic_init(const struct udp_conn_t* conn) {

    struct ossl_quic_data_t* data = (struct ossl_quic_data_t*)conn->data;
    struct ossl_quic_config_t* config = (struct ossl_quic_config_t*)conn->config;

    if(conn == NULL || data == NULL || config == NULL) return -1;

    if(conn->session->mode == 'c') {

        /*
            Configuring SSL context
        */
        data->ctx = SSL_CTX_new(OSSL_QUIC_client_method());
        if(data->ctx == NULL) {
            DEBUG_PRINT("[ERROR] Error while creating ssl context\n");
            return -1;
        }

        SSL_CTX_set_verify(data->ctx, SSL_VERIFY_NONE, NULL);
        
        if(SSL_CTX_set_alpn_protos(data->ctx, alpn_osslquic_own, sizeof(alpn_osslquic_own))) {
            DEBUG_PRINT("[ERROR] Error trying to define alpn proto\n");
            return -1;
        }

        /*
            Creating and binding conn with FD
        */
        data->conn = SSL_new(data->ctx);
        if(data->conn == NULL) {
            DEBUG_PRINT("[ERROR] Error trying to create connection\n");
            return -1;
        }        

        if(!SSL_set_fd(data->conn, conn->session->socket_fd)) {
            DEBUG_PRINT("[DEBUG] Error trying set FD to ssl\n");
            return -1;
        }

        SSL_set_connect_state(data->conn);

        if(connect(conn->session->socket_fd, (struct sockaddr*)&conn->session->dst, sizeof(conn->session->dst)) < 0) {
            DEBUG_PRINT("[ERROR] connect\n");
            return -1;
        }


    } else if(conn->session->mode == 's') {

        /*
            Creating context
        */
        data->ctx = SSL_CTX_new(OSSL_QUIC_server_method());
        if(data->ctx == NULL) {
            DEBUG_PRINT("[ERROR] Error while creating ssl context\n");
            return -1;
        }

        /*
            Loading certs for AUTH
        */
        if(SSL_CTX_use_certificate_file(data->ctx, "../common/server.crt", SSL_FILETYPE_PEM) <= 0) {
            DEBUG_PRINT("[ERROR] Error while trying to load cert file\n");
            return -1;
        }
        
        if(SSL_CTX_use_PrivateKey_file(data->ctx, "../common/server.key", SSL_FILETYPE_PEM) <= 0) {
            DEBUG_PRINT("[ERROR] Error while trying to load key file\n");
            return -1;
        }

        if (!SSL_CTX_check_private_key(data->ctx)) {
            DEBUG_PRINT("[ERROR] Private key does not match cert\n");
            return -1;
        }            

        SSL_CTX_set_min_proto_version(data->ctx, TLS1_3_VERSION);

        /*
            Defining ciphersuites
        */
        if (!ossl_quic_apply_tls13_ciphersuites(data->ctx, config)) {
            DEBUG_PRINT("[ERROR] Error setting ciphersuites\n");
            return -1;
        }

        /*
            Defining aplication layer protocol used (default for osslquic)        
        */
        SSL_CTX_set_alpn_select_cb(data->ctx, ossl_quic_select_alpn, NULL);

        /*
            Configuring socket FD       
        */
        if(setsockopt(conn->session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &config->reuse, sizeof(config->reuse)) < 0) {
            perror("Erro ao configurar setsockopt\n");
            close(conn->session->socket_fd);
            return -1;            
        }

        if(bind(conn->session->socket_fd, (struct sockaddr*)&conn->session->src, sizeof(conn->session->src)) < 0) {
            DEBUG_PRINT("[ERROR] bind %s\n", strerror(errno));
            return -1;            
        }

        /*
            Binding socket FD to OpenSSL
        */
        data->conn = NULL; // vai ser definido depois, quando a connexão for accepted

        data->listener = SSL_new_listener(data->ctx, 0);
        if(data->listener == NULL) {
            DEBUG_PRINT("[ERROR] Error trying to create listener\n");
            return -1;
        }

        if(!SSL_set_fd(data->listener, conn->session->socket_fd)) {
            DEBUG_PRINT("[ERROR] Error trying to set socket fd on ssl listener\n");
            return -1;
        }

        if(!SSL_listen(data->listener)) {
            DEBUG_PRINT("[ERROR] Error trying to listen from ssl_listen()\n");
            return -1;
        }

        if(!SSL_set_blocking_mode(data->listener, 0)) {
            DEBUG_PRINT("[ERROR] Error trying to unset blocking mode\n");
            return -1;
        }

        DEBUG_PRINT("[DEBUG] ossl_quic_init()\n");

        return 0;
    } else {
        DEBUG_PRINT("[Error] Unknown mode\n");
        return -1;
    }
}

static int ossl_quic_deinit(const struct udp_conn_t* conn) {

}

static int ossl_quic_udp_send_ka(const struct udp_conn_t* conn) {

} 

static int ossl_quic_hole_punching(const struct udp_conn_t* conn) {

}

static int ossl_quic_connect(const struct udp_conn_t* conn) {

}

static int ossl_quic_disconnect_send(const struct udp_conn_t* conn) {

}

static int ossl_quic_disconnect_recv(const struct udp_conn_t* conn) {

}

static size_t ossl_quic_udp_send(const struct udp_conn_t* conn, void* buf, size_t nbytes) {

}

static size_t ossl_quic_udp_recv(const struct udp_conn_t* conn) {

}

static int ossl_quic_tcp_bind(const struct udp_conn_t* conn) {

}

static int ossl_quic_tcp_recv(const struct udp_conn_t* conn) {

}

struct udp_conn_generic_api_t ossl_quic_api = {
    .init = ossl_quic_init,
    .deinit = ossl_quic_deinit,
    .hole_punching = ossl_quic_hole_punching,
    .connect = ossl_quic_connect,
    .udp_send = ossl_quic_udp_send,
    .udp_recv = ossl_quic_udp_recv,
    .udp_send_ka = ossl_quic_udp_send_ka,
    .disconnect = ossl_quic_disconnect_send,
    .tcp_bind = ossl_quic_tcp_bind,
    .tcp_recv = ossl_quic_tcp_recv
};