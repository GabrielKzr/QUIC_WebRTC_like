set(TEST_EXTRA_LIBS Libnice::Nice)

if(BUILD_TESTING)
    add_test(NAME nice_local COMMAND test_nice_local)
    set_tests_properties(nice_local PROPERTIES TIMEOUT 10 LABELS "nice;local")
endif()
