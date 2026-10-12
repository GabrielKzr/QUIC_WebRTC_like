cmake_minimum_required(VERSION 3.16)
project(QUIC_WebRTC_Like C)

include(CTest)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/common_functions.cmake")

option(BUILD_NETWORK_TESTS "Build Patchbay network regression tests (Linux and Rust required)" OFF)

if(BUILD_NETWORK_TESTS)
    if(DEFINED TEST_NAME AND NOT "${TEST_NAME}" STREQUAL "")
        message(FATAL_ERROR "BUILD_NETWORK_TESTS requires all peer targets; omit TEST_NAME")
    endif()
    set(BUILD_WEBRTC_TESTS ON)
endif()

add_subdirectory(source)

if(BUILD_NETWORK_TESTS)
    add_subdirectory(sim/patchbay)

endif()
