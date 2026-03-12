cmake_minimum_required(VERSION 3.16)

project(tls_test C)

set(CMAKE_C_STANDARD 11)

add_executable(client client.c)

target_include_directories(client PRIVATE /usr/local/include)

target_link_libraries(client
    mbedtls
    mbedx509
    mbedcrypto
)