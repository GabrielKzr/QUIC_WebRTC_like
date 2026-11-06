set(TEST_EXTRA_LIBS ossl_quic ice_udp_transport)

if(BUILD_TESTING AND BUILD_NETWORK_TESTS)
    check_deps(
        DEPENDENCIES turnserver python3
        HELP_MESSAGE "CMake builds coturn automatically; Python 3 must be available for signaling. See docs/build-dependencies.md"
    )
endif()

set_target_properties(test_quic_ice PROPERTIES
    PATCHBAY_MODE tcp
    PATCHBAY_TIMEOUT 75
    PATCHBAY_TEST_NAME network_quic_ice_tcp
    PATCHBAY_LABELS "network;nice;ice;quic;tcp;stun"
    PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../test_ice_udp_transport/isp.sh"
)
