add_executable(test_chownat_ice_turn
    ../test_chownat_ice/test_chownat_ice.c
)

include("${CMAKE_CURRENT_LIST_DIR}/test.meta.cmake")

target_link_libraries(test_chownat_ice_turn PRIVATE ${TEST_EXTRA_LIBS})
target_compile_definitions(test_chownat_ice_turn PRIVATE ${TEST_EXTRA_DEFINES})
