# WebRTC/test_openssl_quic/test_openssl_quic.meta.cmake

set(TEST_EXTRA_LIBS ossl_quic udp_conn)

set_target_properties(test_openssl_quic PROPERTIES
    PATCHBAY_MODE tcp
    PATCHBAY_TIMEOUT 75
    PATCHBAY_TEST_NAME network_openssl_quic_tcp
    PATCHBAY_LABELS "network;openssl_quic"
)
