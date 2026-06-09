set(TEST_DIRS WebRTC)

foreach(_dir IN LISTS TEST_DIRS)
    add_test_class(${_dir} ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()