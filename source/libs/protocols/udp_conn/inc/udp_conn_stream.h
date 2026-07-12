#include <unistd.h>
#include <stdint.h>

typedef struct udp_conn udp_conn_t;
typedef uint32_t stream_id_t;

typedef enum {
    UDP_CONN_STREAM_OK = 0,
    UDP_CONN_STREAM_ERR = -1,
    UDP_CONN_STREAM_NOT_SUPPORTED = -2, // conexão não suporta streams
    UDP_CONN_STREAM_CLOSED = -3,
    UDP_CONN_STREAM_NOT_FOUND = -4,
} udp_conn_stream_status_t;

typedef struct {
    udp_conn_stream_status_t (*stream_open)(const udp_conn_t*, stream_id_t*);
    udp_conn_stream_status_t (*stream_close)(const udp_conn_t*, stream_id_t);
    udp_conn_stream_status_t (*stream_send)(const udp_conn_t*, stream_id_t, void*, size_t, size_t*);
    udp_conn_stream_status_t (*stream_recv)(const udp_conn_t*, stream_id_t, void*, size_t, size_t*);
} udp_conn_stream_api_t;