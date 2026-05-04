# WebRTC test targets

add_executable(test_chownat
    test_chownat.c
)
target_link_libraries(test_chownat PRIVATE quic_webrtc_lib)
target_compile_definitions(test_chownat PRIVATE APP_CHOWNAT)
target_compile_options(test_chownat PRIVATE -Wall -Wextra -std=c11 -Wno-unused-parameter)

if(ENABLE_OSSLQUIC)
    add_executable(test_openssl_quic
        test_openssl_quic.c
        ${CMAKE_SOURCE_DIR}/src/main.c
    )
    target_link_libraries(test_openssl_quic PRIVATE quic_webrtc_lib OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(test_openssl_quic PRIVATE APP_OPENSSL)
    target_compile_options(test_openssl_quic PRIVATE -Wall -Wextra -std=c11 -Wno-unused-parameter)
endif()
