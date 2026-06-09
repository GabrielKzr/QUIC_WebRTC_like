cmake_minimum_required(VERSION 3.16)

project(QUIC_WebRTC_Like C)

include(cmake_modules/common_defs.cmake)
include(cmake_modules/common_functions.cmake)

add_subdirectory(common)
add_subdirectory(libs)
add_subdirectory(tests)
#add_subdirectory(apps)