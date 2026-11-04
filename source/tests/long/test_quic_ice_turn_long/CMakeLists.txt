add_executable(test_quic_ice_turn_long
    ../../nice/test_quic_ice/test_quic_ice.c
)
include("${CMAKE_CURRENT_LIST_DIR}/test.meta.cmake")
target_link_libraries(test_quic_ice_turn_long PRIVATE ${TEST_EXTRA_LIBS})
target_compile_definitions(test_quic_ice_turn_long PRIVATE ICE_REQUIRE_RELAY ICE_LONG_SESSION)
