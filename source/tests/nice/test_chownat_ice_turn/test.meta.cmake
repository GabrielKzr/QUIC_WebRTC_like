set(TEST_EXTRA_LIBS chownat ice_udp_transport)
set(TEST_EXTRA_DEFINES ICE_REQUIRE_RELAY)

if(BUILD_TESTING AND BUILD_NETWORK_TESTS)
    check_deps(
        DEPENDENCIES turnserver python3
        HELP_MESSAGE "The CHOWNAT TURN test requires coturn and Python 3 in PATH. See source/tests/nice/README.md"
    )
endif()

set_target_properties(test_chownat_ice_turn PROPERTIES
    PATCHBAY_MODE tcp
    PATCHBAY_TIMEOUT 75
    PATCHBAY_TEST_NAME network_chownat_ice_turn_tcp
    PATCHBAY_LABELS "network;nice;ice;chownat;tcp;turn;relay"
    PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../test_ice_udp_transport_turn/isp.sh"
)
