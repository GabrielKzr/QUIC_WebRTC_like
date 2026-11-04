set(TEST_EXTRA_LIBS ossl_quic ice_udp_transport)

if(BUILD_TESTING AND BUILD_NETWORK_TESTS)
    check_deps(DEPENDENCIES turnserver python3
        HELP_MESSAGE "The long QUIC ICE test requires coturn and Python 3 in PATH.")
endif()

set_target_properties(test_quic_ice_turn_long PROPERTIES
    PATCHBAY_MODE tcp-long
    PATCHBAY_TIMEOUT 740
    PATCHBAY_TEST_NAME network_quic_ice_turn_long
    PATCHBAY_LABELS "network;nice;ice;quic;turn;relay;long"
    PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/isp.sh"
)
