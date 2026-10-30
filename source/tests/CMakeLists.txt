set(TEST_DIRS 
    WebRTC
    openssl
)

foreach(_dir IN LISTS TEST_DIRS)
    add_test_class(${_dir} ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

if(BUILD_TESTING)
    add_executable(test_udp_transport unit/test_udp_transport.c)
    target_link_libraries(test_udp_transport PRIVATE udp_conn)
    add_test(NAME unit_udp_transport COMMAND test_udp_transport)
    set_tests_properties(unit_udp_transport PROPERTIES TIMEOUT 5 LABELS "unit;udp_transport")
endif()
