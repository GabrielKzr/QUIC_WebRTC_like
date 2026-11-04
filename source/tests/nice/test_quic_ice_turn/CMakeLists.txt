add_executable(test_quic_ice_turn
    ../test_quic_ice/test_quic_ice.c
)

include("${CMAKE_CURRENT_LIST_DIR}/test.meta.cmake")

target_link_libraries(test_quic_ice_turn PRIVATE ${TEST_EXTRA_LIBS})
target_compile_definitions(test_quic_ice_turn PRIVATE ${TEST_EXTRA_DEFINES})
