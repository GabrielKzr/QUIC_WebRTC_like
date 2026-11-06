cmake_minimum_required(VERSION 3.16)

project(quic_basic_server_client)

if(NOT TARGET OpenSSL::SSL)
    include("${CMAKE_CURRENT_LIST_DIR}/../../../../cmake_modules/common_functions.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/../../../../cmake_modules/openssl.cmake")
endif()

add_executable(server server.c)
add_executable(client client.c)

target_compile_options(server PRIVATE -g -Wall -Wsign-compare)
target_compile_options(client PRIVATE -g -Wall -Wsign-compare)

target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(client PRIVATE OpenSSL::SSL OpenSSL::Crypto)
