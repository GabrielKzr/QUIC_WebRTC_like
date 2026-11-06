include_guard(GLOBAL)
include(ExternalProject)
find_package(Threads REQUIRED)
check_deps(DEPENDENCIES perl make)
set(DEPENDENCY_BUILD_JOBS 4 CACHE STRING "Parallel jobs for external dependency builds")
if(NOT DEPENDENCY_BUILD_JOBS MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "DEPENDENCY_BUILD_JOBS must be a positive integer")
endif()
set(MANAGED_OPENSSL_PREFIX "${CMAKE_BINARY_DIR}/_deps/openssl-4.1.0/install" CACHE INTERNAL "Managed OpenSSL prefix")
ExternalProject_Add(openssl_external
    PREFIX "${CMAKE_BINARY_DIR}/_deps/openssl-4.1.0"
    URL "https://github.com/openssl/openssl/releases/download/openssl-4.1.0/openssl-4.1.0.tar.gz"
    URL_HASH SHA256=50c17baf54cb008f6ce42d50deb4b28050a1aad711636cabd4b241c7e90cc2fd
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads"
    TLS_VERIFY TRUE
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env "CC=${CMAKE_C_COMPILER}"
        "${PERL_EXECUTABLE}" <SOURCE_DIR>/Configure
        "--prefix=${MANAGED_OPENSSL_PREFIX}" --libdir=lib no-shared no-tests -fPIC
    BUILD_COMMAND "${MAKE_EXECUTABLE}" "-j${DEPENDENCY_BUILD_JOBS}"
    INSTALL_COMMAND "${MAKE_EXECUTABLE}" install_sw
    BUILD_BYPRODUCTS "${MANAGED_OPENSSL_PREFIX}/lib/libssl.a" "${MANAGED_OPENSSL_PREFIX}/lib/libcrypto.a"
    LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
)
file(MAKE_DIRECTORY "${MANAGED_OPENSSL_PREFIX}/include")
add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION "${MANAGED_OPENSSL_PREFIX}/lib/libcrypto.a"
    INTERFACE_INCLUDE_DIRECTORIES "${MANAGED_OPENSSL_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES "Threads::Threads;${CMAKE_DL_LIBS}")
add_dependencies(OpenSSL::Crypto openssl_external)
add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
set_target_properties(OpenSSL::SSL PROPERTIES
    IMPORTED_LOCATION "${MANAGED_OPENSSL_PREFIX}/lib/libssl.a"
    INTERFACE_INCLUDE_DIRECTORIES "${MANAGED_OPENSSL_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES OpenSSL::Crypto)
add_dependencies(OpenSSL::SSL openssl_external)
