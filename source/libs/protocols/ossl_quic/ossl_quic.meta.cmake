# protocols/ossl_quic/ossl_quic.meta.cmake

set(OPENSSL_ROOT_DIR "$ENV{HOME}/.local/openssl")

find_package(OpenSSL REQUIRED)

set(LIB_EXTRA_LIBS    OpenSSL::SSL OpenSSL::Crypto)
set(LIB_EXTRA_INCLUDES ${OPENSSL_INCLUDE_DIR})