find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNICE REQUIRED IMPORTED_TARGET nice)

set(TEST_EXTRA_LIBS PkgConfig::LIBNICE)

if(BUILD_TESTING)
    add_test(NAME nice_local COMMAND test_nice_local)
    set_tests_properties(nice_local PROPERTIES TIMEOUT 10 LABELS "nice;local")
endif()
