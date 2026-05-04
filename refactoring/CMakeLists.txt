cmake_minimum_required(VERSION 3.16)

project(QUIC_WebRTC_Like C)

# Biblioteca estática com os componentes comuns
set(COMMON_SRCS
    src/utils.c
    src/udp_conn.c
    src/chownat.c
    src/quic_like.c
)

add_library(quic_webrtc_lib STATIC ${COMMON_SRCS})

target_include_directories(quic_webrtc_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_options(quic_webrtc_lib PRIVATE -Wall -Wextra -std=c11 -Wno-unused-parameter)

# Opcional: suporte a OpenSSL QUIC
option(ENABLE_OSSLQUIC "Enable OpenSSL QUIC support" OFF)

if(ENABLE_OSSLQUIC)
    find_package(OpenSSL REQUIRED)
    target_sources(quic_webrtc_lib PRIVATE src/ossl_quic.c)
    target_link_libraries(quic_webrtc_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(quic_webrtc_lib PUBLIC APP_OPENSSL)
endif()

# Adiciona subdirectório de testes
add_subdirectory(tests)