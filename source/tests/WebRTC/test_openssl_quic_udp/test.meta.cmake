# WebRTC/test_openssl_quic_udp/test_openssl_quic_udp.meta.cmake

set(TEST_EXTRA_LIBS ossl_quic udp_conn)

set_target_properties(test_openssl_quic_udp PROPERTIES
    PATCHBAY_MODE udp
    PATCHBAY_TIMEOUT 20
    PATCHBAY_TEST_NAME network_openssl_quic_udp
    PATCHBAY_LABELS "network;openssl_quic"
)
