set(TEST_EXTRA_LIBS ice_udp_transport)

if(BUILD_TESTING)
    check_deps(
        DEPENDENCIES turnserver
        HELP_MESSAGE "CMake builds coturn automatically for the ICE failure tests. See docs/build-dependencies.md"
    )
    add_dependencies(test_ice_failures coturn_external)
    foreach(scenario signaling_unavailable turn_bad_credentials remote_absent turn_stale_nonce)
        add_test(NAME ice_failure_${scenario} COMMAND test_ice_failures ${scenario})
        managed_test_environment(ice_failure_${scenario})
        set_tests_properties(ice_failure_${scenario} PROPERTIES
            TIMEOUT 15 LABELS "nice;ice;local;failure")
    endforeach()
endif()
