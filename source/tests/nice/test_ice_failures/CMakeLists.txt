add_executable(test_ice_failures test_ice_failures.c)
include("${CMAKE_CURRENT_LIST_DIR}/test.meta.cmake")
target_link_libraries(test_ice_failures PRIVATE ${TEST_EXTRA_LIBS})
