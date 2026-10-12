# WebRTC/test_chownat/test_chownat.meta.cmake

set(TEST_EXTRA_LIBS  chownat udp_conn)

set_target_properties(test_chownat PROPERTIES
    PATCHBAY_MODE tcp
    PATCHBAY_TIMEOUT 75
    PATCHBAY_TEST_NAME network_chownat_tcp
    PATCHBAY_LABELS "network;chownat"
)
