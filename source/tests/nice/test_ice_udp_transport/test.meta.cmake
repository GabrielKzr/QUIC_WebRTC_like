set(TEST_EXTRA_LIBS ice_udp_transport)

if(BUILD_TESTING AND BUILD_NETWORK_TESTS)
    check_deps(
        DEPENDENCIES turnserver python3
        HELP_MESSAGE "The ICE network test requires coturn (turnserver) and Python 3 in PATH. See source/tests/nice/README.md"
    )
endif()

set_target_properties(test_ice_udp_transport PROPERTIES
    PATCHBAY_MODE udp
    PATCHBAY_TIMEOUT 20
    PATCHBAY_TEST_NAME network_ice_udp_transport
    PATCHBAY_LABELS "network;nice;ice;stun"
    PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/isp.sh"
)
