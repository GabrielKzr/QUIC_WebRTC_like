add_executable(test_ice_udp_transport_remap
    ../test_ice_udp_transport/test_ice_udp_transport.c
)

include("${CMAKE_CURRENT_LIST_DIR}/test.meta.cmake")

target_link_libraries(test_ice_udp_transport_remap PRIVATE ${TEST_EXTRA_LIBS})
target_compile_definitions(test_ice_udp_transport_remap PRIVATE ${TEST_EXTRA_DEFINES})
set_property(TARGET test_ice_udp_transport_remap PROPERTY
    PATCHBAY_NAT_EXTRA_RULES "${PATCHBAY_NAT_EXTRA_RULES}"
)
