set(TEST_EXTRA_LIBS ice_udp_transport)
set(TEST_EXTRA_DEFINES ICE_REQUIRE_RELAY)

if(BUILD_TESTING AND BUILD_NETWORK_TESTS)
    check_deps(
        DEPENDENCIES turnserver python3
        HELP_MESSAGE "CMake builds coturn automatically; Python 3 must be available for signaling. See docs/build-dependencies.md"
    )
endif()

set_target_properties(test_ice_udp_transport_turn PROPERTIES
    PATCHBAY_MODE udp
    PATCHBAY_TIMEOUT 20
    PATCHBAY_TEST_NAME network_ice_udp_transport_turn
    PATCHBAY_LABELS "network;nice;ice;turn;relay"
    PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/isp.sh"
)
