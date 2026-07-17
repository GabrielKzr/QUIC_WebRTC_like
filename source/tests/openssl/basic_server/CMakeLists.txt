cmake_minimum_required(VERSION 3.16)

project(quic_basic_server_client)

# Forçar uso da versão 4.1.0-dev do OpenSSL
set(OPENSSL_ROOT_DIR "/usr/local")
set(OPENSSL_INCLUDE_DIR "/usr/local/include")
set(OPENSSL_SSL_LIBRARY "/usr/local/lib64/libssl.so")
set(OPENSSL_CRYPTO_LIBRARY "/usr/local/lib64/libcrypto.so")

add_executable(server server.c)
add_executable(client client.c)

target_include_directories(server PRIVATE ${OPENSSL_INCLUDE_DIR})
target_include_directories(client PRIVATE ${OPENSSL_INCLUDE_DIR})

target_compile_options(server PRIVATE -g -Wall -Wsign-compare)
target_compile_options(client PRIVATE -g -Wall -Wsign-compare)

target_link_libraries(server PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(client PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})