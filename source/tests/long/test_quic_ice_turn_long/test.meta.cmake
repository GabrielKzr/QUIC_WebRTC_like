set(TEST_EXTRA_LIBS ossl_quic ice_udp_transport)

if(BUILD_TESTING AND BUILD_NETWORK_TESTS)
    check_deps(DEPENDENCIES turnserver python3
        HELP_MESSAGE "CMake builds coturn automatically; Python 3 must be available for signaling. See docs/build-dependencies.md")
endif()

set_target_properties(test_quic_ice_turn_long PROPERTIES
    PATCHBAY_MODE tcp-long
    PATCHBAY_TIMEOUT 740
    PATCHBAY_TEST_NAME network_quic_ice_turn_long
    PATCHBAY_LABELS "network;nice;ice;quic;turn;relay;long"
    PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/isp.sh"
)
