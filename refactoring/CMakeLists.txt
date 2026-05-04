cmake_minimum_required(VERSION 3.16)

project(QUIC_WebRTC_Like C)

set(APP_SRC "")
set(ENABLE_OSSLQUIC OFF)

if(APP STREQUAL "CHOWNAT")
    set(APP_SRC test_examples/WebRTC/test_chownat.c)
    add_compile_definitions(APP_CHOWNAT)

elseif(APP STREQUAL "OPENSSL_QUIC")
    set(APP_SRC test_examples/WebRTC/test_openssl_quic.c)
    add_compile_definitions(APP_OPENSSL)
    set(ENABLE_OSSLQUIC ON)

endif()

set(SRCS
    src/main.c
    src/utils.c
    src/udp_conn.c
    src/chownat.c
    src/quic_like.c
)

if(ENABLE_OSSLQUIC)
    list(APPEND SRCS src/ossl_quic.c)
    find_package(OpenSSL REQUIRED)
endif()

add_executable(main ${SRCS} ${APP_SRC})

target_include_directories(main PRIVATE
    inc
)

if(ENABLE_OSSLQUIC)
    target_link_libraries(main PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

target_compile_options(main PRIVATE -Wall -Wextra -std=c11 -Wno-unused-parameter)