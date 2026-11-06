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

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
option(BUILD_PCAP_TOOLS "Build namespace-compatible tcpdump and libpcap" OFF)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/openssl.cmake")
if(BUILD_PCAP_TOOLS)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/pcap.cmake")
endif()

add_subdirectory(source)

if(BUILD_NETWORK_TESTS)
    add_subdirectory(sim/patchbay)
endif()
