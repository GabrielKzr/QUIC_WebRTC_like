cmake_minimum_required(VERSION 3.16)

project(quic_basic_server_client)

set(OPENSSL_ROOT_DIR "$ENV{HOME}/.local/openssl")
find_package(OpenSSL REQUIRED)

add_executable(server server.c)
add_executable(client client.c)

target_compile_options(server PRIVATE -g -Wall -Wsign-compare)
target_compile_options(client PRIVATE -g -Wall -Wsign-compare)

target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(client PRIVATE OpenSSL::SSL OpenSSL::Crypto)
