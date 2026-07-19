#include <unistd.h>
#include "utils.h"
#include "chownat.h"

void udp_conn_calback(const udp_conn_t* conn, int reason, void* data_in, size_t nbytes);

chownat_data_t chownat_data = {0};

chownat_config_t chownat_config = {
    .udp_recv_timeout_sec = 1,
    .reuse = 1,
    .conn_max_attempts = 20,
    .dconn_max_attempts = 20
};

/* ===================== SESSION ===================== */

static udp_session_t udp_session = {0};
// static tcp_session_t tcp_session = {0};
static udp_conn_ctrl_t ctrl = {0};

/* ===================== CONTEXT ===================== */

static udp_conn_t _conn = {
    .udp_session = &udp_session,
    .tcp_session = NULL,
    .ctrl = &ctrl,
    .config = &chownat_config,
    .data = &chownat_data,
    .api = &chownat_api,
    .udp_conn_callback = udp_conn_calback
};

static udp_conn_t *conn = &_conn;

/* ===================== CALLBACK ===================== */

void udp_conn_calback(const udp_conn_t* conn, int reason, void* data_in, size_t nbytes)
{
    if (conn->tcp_session)
        return;

    switch (reason)
    {
        case CHOWNAT_UDP_CONNECTED:
        {
            DEBUG_PRINT("[DEBUG] Connected Successfully\n");

            const char *msg = "Hello World!";
            size_t sent_bytes = 0;
            udp_conn_send(conn, (void*)msg, strlen(msg), &sent_bytes);

            break;
        }

        case CHOWNAT_UDP_RECV_DATA:
        {
            char *msg = (char *)data_in;
            DEBUG_PRINT("[DEBUG] Received Data [%s]\n", msg);

            chownat_data_t* data = (chownat_data_t*)conn->data;

            char buf[64] = {0};
            size_t send_bytes = 0;

            if (conn->ctrl && conn->ctrl->mode == 'c')
            {
                send_bytes = snprintf(buf, sizeof(buf),
                                      "%d: Hello from client",
                                      data->expected);
            }
            else
            {
                send_bytes = snprintf(buf, sizeof(buf),
                                      "%d: Hello from server",
                                      data->expected);
            }

            size_t sent_bytes = 0;
            udp_conn_send(conn, buf, send_bytes, &sent_bytes);

            // 100 milliseconds delay
            usleep(100000);

            // if is client, then close the connection after 20 messages
            if (conn->ctrl && conn->ctrl->mode == 'c')
            {
                if (data->expected >= 20)
                {
                    DEBUG_PRINT("[DEBUG] Limite de 20 mensagens atingido. Desconectando cliente...\n");
                    udp_conn_disconnect(conn);
                }
            }

            break;
        }

        case CHOWNAT_UDP_LOST_DATA:
            DEBUG_PRINT("[DEBUG] Lost Data\n");
            break;

        case CHOWNAT_TCP_DATA_SENT:
            DEBUG_PRINT("[DEBUG] TCP Data Sent\n");
            break;

        default:
            DEBUG_PRINT("[DEBUG] Unknown Reason\n");
            break;
    }
}

/* ===================== MAIN ===================== */

int main(int argc, char *argv[])
{
    int DEBUG, remoteport;
    char *mode, *remoteaddr;

    usage(argc, argv, &DEBUG, &mode, NULL, &remoteaddr, &remoteport);

    DEBUG_PRINT("[DEBUG] Mode: %s\n", mode);
    DEBUG_PRINT("[DEBUG] Remote address: %s\n", remoteaddr);
    DEBUG_PRINT("[DEBUG] Remote port: %d\n", remoteport);
    DEBUG_PRINT("[DEBUG] Debug level: %d\n", DEBUG);

    /* ===================== SOCKETS ===================== */

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        perror("udp socket");
        exit(EXIT_FAILURE);
    }

    /* ===================== ADDR CONFIG ===================== */

    struct sockaddr_in dst = {0};
    struct sockaddr_in src = {0};

    dst.sin_family = AF_INET;
    dst.sin_port = htons(remoteport);
    dst.sin_addr.s_addr = inet_addr(remoteaddr);

    src.sin_family = AF_INET;
    src.sin_port = htons(remoteport);
    src.sin_addr.s_addr = INADDR_ANY;

    /* ===================== SESSION INIT ===================== */

    udp_session.socket_fd = udp_fd;
    udp_session.dst = dst;
    udp_session.src = src;
    udp_session.ka_miss_threshold = 20;

    ctrl.mode = mode[1];
    ctrl.init = 0;
    FD_ZERO(&ctrl.read_fds);

    /* ===================== INIT FLOW ===================== */

    DEBUG_PRINT("starting init\n");

    if (udp_conn_init(conn) != 0) {
        fprintf(stderr, "udp_conn_init failed\n");
        exit(EXIT_FAILURE);
    }

    print_sockaddr_in(&conn->udp_session->dst);

    if (udp_connection(conn) != 0) {
        fprintf(stderr, "udp_connection failed\n");
    }

    udp_conn_deinit(conn);

    return 0;
}