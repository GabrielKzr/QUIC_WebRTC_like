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
static int drain_socket_rx_queue(int fd);
static udp_conn_status_t ossl_quic_connect(const udp_conn_t* conn);
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

    /*
        Configuring socket FD       
    */
    if(setsockopt(udp_session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &config->reuse, sizeof(config->reuse)) < 0) {
        DEBUG_PRINT("[ERROR] Error while configuring setsockopt\n");
        close(udp_session->socket_fd);
        return UDP_CONN_ERR;            
    }

    if(bind(udp_session->socket_fd, (struct sockaddr*)&udp_session->src, sizeof(udp_session->src)) < 0) {
        DEBUG_PRINT("[ERROR] bind %s\n", strerror(errno));
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
    tcp_session_t* tcp_session = conn->tcp_session;

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(!ctrl->init) return UDP_CONN_NOT_INIT;

    // Um erro de fechamento nao pode impedir a liberacao dos recursos.
    if (!data->closed)
        ossl_quic_disconnect(conn);

    SSL_free(data->conn);
    data->conn = NULL;
    SSL_free(data->listener);
    data->listener = NULL;
    SSL_CTX_free(data->ctx);
    data->ctx = NULL;

    if (udp_session->socket_fd >= 0) {
        close(udp_session->socket_fd);
        udp_session->socket_fd = -1;
    }
    if (tcp_session != NULL) {
        if (tcp_session->accepted_sock >= 0) {
            close(tcp_session->accepted_sock);
            tcp_session->accepted_sock = -1;
        }
        if (tcp_session->socket_fd >= 0) {
            close(tcp_session->socket_fd);
            tcp_session->socket_fd = -1;
        }
    }
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

#if 0
static udp_conn_status_t ossl_quic_hole_punching(const udp_conn_t* conn) {
    /* Experimental wire format, not STUN: probe, response, ready. */
    const char probe[] = "01\n";
    const char ack[] = "03\n";
    const char ready_msg[] = "04\n";
    const int64_t budget_ms = 3000;
    int64_t rto_ms = 500;
    int confirmed = 0;
    int peer_ready = 0;
    int triggered_check = 0;

    if (conn == NULL || conn->udp_session == NULL) return UDP_CONN_ERR;

    udp_session_t* session = conn->udp_session;
    int fd = session->socket_fd;
    if (fd < 0 || fd >= FD_SETSIZE) return UDP_CONN_ERR;

    int64_t now = ossl_quic_monotonic_ms();
    if (now < 0) return UDP_CONN_ERR;
    int64_t deadline = now + budget_ms;
    int64_t next_send = now;
    int sent_request = 0;

    while (1) {
        now = ossl_quic_monotonic_ms();
        if (now < 0) return UDP_CONN_ERR;
        if (now >= deadline) {
            DEBUG_PRINT("[ERROR] Hole punching deadline expired\n");
            return UDP_CONN_ERR;
        }

        if (now >= next_send) {
            const char* msg = confirmed ? ready_msg : probe;
            ssize_t sent = sendto(fd, msg, 3, MSG_DONTWAIT,
                                  (struct sockaddr*)&session->dst,
                                  sizeof(session->dst));
            if (sent < 0) {
                if (errno == EINTR) continue;
                if (errno != EAGAIN && errno != EWOULDBLOCK) return UDP_CONN_ERR;
                next_send = now + rto_ms;
            } else {
                if (sent != 3) return UDP_CONN_ERR;
                if (confirmed && peer_ready) return UDP_CONN_OK;
                if (sent_request) rto_ms *= 2;
                sent_request = 1;
                next_send = now + rto_ms;
            }
        }

        int64_t wake_at = next_send < deadline ? next_send : deadline;
        int64_t remaining = wake_at - now;
        if (remaining < 0) remaining = 0;
        struct timeval timeout = {
            .tv_sec = remaining / 1000,
            .tv_usec = (remaining % 1000) * 1000
        };
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        int nready = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if (nready < 0) {
            if (errno == EINTR) continue;
            return UDP_CONN_ERR;
        }
        if (nready == 0) continue;

        char buffer[4];
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        ssize_t nread = recvfrom(fd, buffer, sizeof(buffer), MSG_DONTWAIT,
                                 (struct sockaddr*)&sender, &sender_len);
        if (nread < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return UDP_CONN_ERR;
        }
        if (sender_len != sizeof(sender) ||
            sender.sin_family != session->dst.sin_family ||
            sender.sin_addr.s_addr != session->dst.sin_addr.s_addr ||
            sender.sin_port != session->dst.sin_port || nread != 3) continue;

        if (memcmp(buffer, probe, 3) == 0) {
            /* A response is immediate; RTO only governs our own retries. */
            ssize_t sent = sendto(fd, ack, 3, MSG_DONTWAIT,
                                  (struct sockaddr*)&session->dst,
                                  sizeof(session->dst));
            if (sent < 0) {
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                return UDP_CONN_ERR;
            }
            if (sent != 3) return UDP_CONN_ERR;
        } else if (memcmp(buffer, ack, 3) == 0) {
            if (!confirmed) {
                confirmed = 1;
                rto_ms = 500;
                sent_request = 0;
                next_send = 0; /* Advertise readiness without waiting for RTO. */
            }
        } else if (memcmp(buffer, ready_msg, 3) == 0) {
            peer_ready = 1;
            if (confirmed) {
                next_send = 0; /* Final READY also helps a lost READY. */
            } else if (!triggered_check) {
                /* Peer progress triggers one check, without shortening RTO. */
                triggered_check = 1;
                rto_ms = 500;
                sent_request = 0;
                next_send = 0;
            }
        }
    }
}
#endif /* 0 */

static udp_conn_status_t ossl_quic_hole_punching(const udp_conn_t* conn) {
    uint8_t hole_punched = 0;
    const char msg[] = "01\n";
    char buffer[4] = {0};
    ssize_t nread;
    socklen_t dst_len;

    udp_session_t* udp_session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if (udp_session == NULL || ctrl == NULL) return UDP_CONN_ERR;
    if (ctrl->mode != 'c' && ctrl->mode != 's') return UDP_CONN_UNKNOWN_MODE;

    dst_len = sizeof(udp_session->dst);

    /* IDLE: timeout de 1s para fase de descoberta */
    struct timeval udp_recv_timeout = {
        .tv_sec = 1,
        .tv_usec = 0
    };

    if (setsockopt(udp_session->socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                   &udp_recv_timeout, sizeof(udp_recv_timeout)) < 0) {
        DEBUG_PRINT("[ERROR] Erro ao configurar SO_RCVTIMEO: %s\n", strerror(errno));
        return UDP_CONN_ERR;
    }

    DEBUG_PRINT("[DEBUG] Trying a connection with remote end\n");

    /* Only the client initiates discovery; the server waits for its trigger. */
    while (1) {

        DEBUG_PRINT("[DEBUG] Attempting to connect\n");
        
        if (ctrl->mode == 'c' &&
            sendto(udp_session->socket_fd, msg, sizeof(msg) - 1, 0, (struct sockaddr*)&udp_session->dst, dst_len) < 0) {
            
            if (errno == EINTR) continue;
            
            DEBUG_PRINT("[ERROR] sendto failed: %s\n", strerror(errno));
            return UDP_CONN_ERR;
        }

        nread = recv(udp_session->socket_fd, buffer, 3, 0);
        if (nread < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (ctrl->mode == 's') {
                    /* Raw UDP keep-alive opens the NAT mapping before QUIC exists. */
                    ssize_t sent;
                    do {
                        sent = sendto(udp_session->socket_fd, "\0", 1, 0,
                                      (struct sockaddr*)&udp_session->dst, dst_len);
                    } while (sent < 0 && errno == EINTR);
                    if (sent != 1) {
                        DEBUG_PRINT("[ERROR] Hole punching keep-alive failed\n");
                        return UDP_CONN_ERR;
                    }
                }
                DEBUG_PRINT("[DEBUG] Timeout\n");
                continue; /* timeout/interrupção: tenta de novo */
            }
            DEBUG_PRINT("[ERROR] recv failed: %s\n", strerror(errno));
            return UDP_CONN_ERR;
        }

        if (nread == 3 && memcmp(buffer, msg, 3) == 0) {
            DEBUG_PRINT("[REMOTE] Discovered a remote end\n");
            break;
        }

        DEBUG_PRINT("[ERROR] Should not be here\n");
    }

    /* TRANSIENT: timeout curto + recv não bloqueante */
    for (int i = 0; i < 10; i++) {
        if (sendto(udp_session->socket_fd, msg, sizeof(msg) - 1, 0,
                   (struct sockaddr*)&udp_session->dst, dst_len) < 0) {
            if (errno == EINTR) continue;
            DEBUG_PRINT("[ERROR] sendto failed on transient: %s\n", strerror(errno));
            return UDP_CONN_ERR;
        }

        msleep(50);

        if (!hole_punched) {
            nread = recv(udp_session->socket_fd, buffer, 3, MSG_DONTWAIT);
            if (nread >= 0) {
                if (nread == 3 && memcmp(buffer, msg, 3) == 0) {
                    hole_punched = 1;
                }
            } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                DEBUG_PRINT("[ERROR] recv failed on transient: %s\n", strerror(errno));
                return UDP_CONN_ERR;
            }
        }
    }

    udp_recv_timeout.tv_sec = 0;
    udp_recv_timeout.tv_usec = 0;

    if (setsockopt(udp_session->socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                   &udp_recv_timeout, sizeof(udp_recv_timeout)) < 0) {
        DEBUG_PRINT("[ERROR] Erro ao configurar SO_RCVTIMEO: %s\n", strerror(errno));
        return UDP_CONN_ERR;
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

    if(udp_session == NULL) return UDP_CONN_ERR;
    if(data == NULL)        return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    if(ctrl->mode == 'c') {

        /*
            Creating and binding conn with FD
        */
        data->conn = SSL_new(data->ctx);
        if(data->conn == NULL) {
            DEBUG_PRINT("[ERROR] Error trying to create connection\n");
            return UDP_CONN_ERR;
        }        

        if(!SSL_set_fd(data->conn, udp_session->socket_fd)) {
            DEBUG_PRINT("[DEBUG] Error trying set FD to ssl\n");
            return UDP_CONN_ERR;
        }

        SSL_set_connect_state(data->conn);

        if(connect(udp_session->socket_fd, (struct sockaddr*)&udp_session->dst, sizeof(udp_session->dst)) < 0) {
            DEBUG_PRINT("[ERROR] connect\n");
            return UDP_CONN_ERR;
        }

    } else if(ctrl->mode == 's') {

        /*
            Binding socket FD to OpenSSL
        */
        data->conn = NULL; // vai ser definido depois, quando a connexão for accepted

        data->listener = SSL_new_listener(data->ctx, 0);
        if(data->listener == NULL) {
            DEBUG_PRINT("[ERROR] Error trying to create listener\n");
            return UDP_CONN_ERR;
        }

        if(!SSL_set_fd(data->listener, udp_session->socket_fd)) {
            DEBUG_PRINT("[ERROR] Error trying to set socket fd on ssl listener\n");
            return UDP_CONN_ERR;
        }

        if(!SSL_listen(data->listener)) {
            DEBUG_PRINT("[ERROR] Error trying to listen from ssl_listen()\n");
            return UDP_CONN_ERR;
        }

        if(!SSL_set_blocking_mode(data->listener, 1)) {
            DEBUG_PRINT("[ERROR] Error trying to unset blocking mode\n");
            return UDP_CONN_ERR;
        }

    } else {
        DEBUG_PRINT("[DEBUG] Unknown mode\n");
    }

    DEBUG_PRINT("[DEBUG] Socket FD binded with OpenSSL QUIC\n");

    return UDP_CONN_OK;
}

#if 0
static int drain_socket_rx_queue(int fd) {
    char trash[2048];
    int drained = 0;

    for (;;) {
        ssize_t n = recv(fd, trash, sizeof(trash), MSG_DONTWAIT);
        if (n > 0) {
            drained++;
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;

        if (errno == EINTR)
            continue;

        return -1;
    }

    return drained;
}
#endif /* 0 */

static udp_conn_status_t ossl_quic_connect(const udp_conn_t* conn) {

    ossl_quic_data_t* data = conn->data;
    udp_session_t* udp_session = conn->udp_session;
    udp_conn_ctrl_t* ctrl = conn->ctrl;

    if(data == NULL)        return UDP_CONN_ERR;
    if(udp_session == NULL) return UDP_CONN_ERR;
    if(ctrl == NULL)        return UDP_CONN_ERR;

    // bind socket FD to OpenSSL QUIC
    ossl_quic_pre_connect(conn);

    if(ctrl->mode == 'c') {
        
        if(!SSL_connect(data->conn)) {
            DEBUG_PRINT("[ERROR] Error trying to connect to remote end through QUIC\n");
            ERR_print_errors_fp(stderr);
            return UDP_CONN_ERR;
        }
        
        DEBUG_PRINT("[DEBUG] Connected to QUIC server\n");

    } else if(ctrl->mode == 's') {

        DEBUG_PRINT("[DEBUG] QUIC server listening\n");

        data->conn = SSL_accept_connection(data->listener, 0);

        if(data->conn == NULL) {
            DEBUG_PRINT("[ERROR] Error trying to connect to remote end through QUIC\n");
            ERR_print_errors_fp(stderr);
            return UDP_CONN_ERR;
        }

        DEBUG_PRINT("[DEBUG] Connected to QUIC client\n");

    } else {
        DEBUG_PRINT("[ERROR] Unknown mode\n");
        return UDP_CONN_ERR;
    }

    if(!SSL_set_blocking_mode(data->conn, 0)) { 
        DEBUG_PRINT("[ERROR] Error setting non-blocking mode\n");
        return UDP_CONN_ERR;
    }

    conn->udp_conn_callback(conn, OSSL_QUIC_UDP_CONNECTED, NULL, 0);

    data->closed = 0;

    return UDP_CONN_OK;
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
    
    DEBUG_PRINT("[DEBUG] Ignore keep-alive sent (QUIC does not need it)\n");

    return UDP_CONN_OK;

    if(!SSL_write(data->conn, "\0", 1)) {
        ERR_print_errors_fp(stderr);
        exit(errno);
    }
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

static int64_t ossl_quic_monotonic_ms(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
        return -1;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static udp_conn_status_t ossl_quic_finish_shutdown(const udp_conn_t* conn)
{
    ossl_quic_data_t* data = conn->data;
    int fd = conn->udp_session->socket_fd;
    int64_t started = ossl_quic_monotonic_ms();
    if (started < 0 || fd < 0 || fd >= FD_SETSIZE)
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

        // checks the shutdown timeout, if exceeded return error, otherwise wait for events
        int64_t now = ossl_quic_monotonic_ms();
        if (now < 0)
            return UDP_CONN_ERR;
        if (now >= deadline) {
            fprintf(stderr, "QUIC shutdown exceeded 5 seconds\n");
            return UDP_CONN_ERR;
        }

        // check next event to be handled and wait for it. Time is choosen between 
        // the next event and the shutdown timeout, whichever is smaller. 
        struct timeval event_timeout = {0};
        int infinite = 0;
        if (!SSL_get_event_timeout(data->conn, &event_timeout, &infinite))
            return UDP_CONN_ERR;
        int64_t wait_ms = deadline - now;
        if (!infinite) {
            int64_t event_ms = (int64_t)event_timeout.tv_sec * 1000 +
                               (event_timeout.tv_usec + 999) / 1000;
            if (event_ms < wait_ms)
                wait_ms = event_ms;
        }
        if (wait_ms < 0)
            wait_ms = 0;

        // Wait for events on the socket, if some are ready, loop and handle them. If timeout,
        // loop and try to shutdown again. if shutdown timeout exceeded, it will check in
        // the next loop and return error.
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(fd, &read_fds);

        int want_write = SSL_net_write_desired(data->conn);
        if (want_write)
            FD_SET(fd, &write_fds);

        struct timeval timeout = {
            .tv_sec = wait_ms / 1000,
            .tv_usec = (wait_ms % 1000) * 1000
        };
        int ready = select(fd + 1, &read_fds,
                           want_write ? &write_fds : NULL, NULL, &timeout);
        if (ready < 0 && errno != EINTR)
            return UDP_CONN_ERR;
        // EINTR volta ao inicio, recalculando o prazo monotonicamente.
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

    DEBUG_PRINT("[DEBUG] Sending %zu bytes through QUIC\n", nbytes);

    if (!SSL_write_ex(data->conn, buf, nbytes, &sent)) {
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

    int ret = SSL_read_ex(data->conn, buf, nbytes, recv_bytes);
    if (ret == 1) {
        conn->udp_conn_callback(conn, OSSL_QUIC_UDP_RECV_DATA, buf, *recv_bytes);
        return UDP_CONN_OK;
    }

    int err = SSL_get_error(data->conn, ret);
    *recv_bytes = 0;

    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
        return UDP_CONN_OK;
    return UDP_CONN_ERR;
}

static size_t ossl_quic_pending_data(const udp_conn_t *conn) {
    ossl_quic_data_t *data = conn->data;
    return (size_t)SSL_pending(data->conn);
}

static udp_conn_status_t ossl_quic_process_events(const udp_conn_t *conn) {

    ossl_quic_data_t *data = conn->data;

    if (data == NULL || data->conn == NULL) return UDP_CONN_ERR;
    if (!SSL_handle_events(data->conn)) return UDP_CONN_ERR;
    if (ossl_quic_record_close_status(data))
        return ossl_quic_disconnect(conn);

    return UDP_CONN_OK;
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
