cmake_minimum_required(VERSION 3.28.3)

project(QUIC_WebRTC_Like)

set(SRCS
    src/main.c
    src/utils.c
    src/udp_conn.c
    src/chownat.c
    src/quic_like.c
    src/ossl_quic.c
)

set(OPENSSL_ROOT_DIR "/usr/local")
set(OPENSSL_INCLUDE_DIR "/usr/local/include")
set(OPENSSL_SSL_LIBRARY "/usr/local/lib64/libssl.so")
set(OPENSSL_CRYPTO_LIBRARY "/usr/local/lib64/libcrypto.so")

set(INCLUDES
    inc
    ${OPENSSL_INCLUDE_DIR}
)

add_executable(main ${SRCS})

target_include_directories(main PRIVATE ${INCLUDES})

target_compile_options(main PRIVATE -Wall -Wextra -std=c11 -Wno-unused-parameter)

target_link_libraries(main PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})