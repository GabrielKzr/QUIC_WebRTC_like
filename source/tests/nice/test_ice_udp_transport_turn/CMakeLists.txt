add_executable(test_ice_udp_transport_turn
    ../test_ice_udp_transport/test_ice_udp_transport.c
)

include("${CMAKE_CURRENT_LIST_DIR}/test.meta.cmake")

target_link_libraries(test_ice_udp_transport_turn PRIVATE ${TEST_EXTRA_LIBS})
target_compile_definitions(test_ice_udp_transport_turn PRIVATE ${TEST_EXTRA_DEFINES})
