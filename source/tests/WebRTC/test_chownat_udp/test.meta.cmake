# WebRTC/test_chownat_udp/test.meta.cmake

set(TEST_EXTRA_LIBS  chownat direct_udp_transport)

set_target_properties(test_chownat_udp PROPERTIES
    PATCHBAY_MODE udp
    PATCHBAY_TIMEOUT 20
    PATCHBAY_TEST_NAME network_chownat_udp
    PATCHBAY_LABELS "network;chownat;baseline"
)
