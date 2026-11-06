include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/openssl.cmake")
set(_event_prefix "${CMAKE_BINARY_DIR}/_deps/libevent-2.1.12/install")
ExternalProject_Add(libevent_external
    PREFIX "${CMAKE_BINARY_DIR}/_deps/libevent-2.1.12"
    URL "https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz"
    URL_HASH SHA256=92e6de1be9ec176428fd2367677e61ceffc2ee1cb119035037a27d346b0403bb
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads" TLS_VERIFY TRUE
    DEPENDS openssl_external
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env
        "PKG_CONFIG_PATH=${MANAGED_OPENSSL_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
        "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" -S <SOURCE_DIR> -B <BINARY_DIR>
        -DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON "-DCMAKE_INSTALL_PREFIX=${_event_prefix}" -DCMAKE_INSTALL_LIBDIR=lib
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DEVENT__LIBRARY_TYPE=STATIC
        -DEVENT__DISABLE_TESTS=ON -DEVENT__DISABLE_REGRESS=ON
        -DEVENT__DISABLE_SAMPLES=ON -DEVENT__DISABLE_BENCHMARK=ON
        "-DOPENSSL_ROOT_DIR=${MANAGED_OPENSSL_PREFIX}" -DOPENSSL_USE_STATIC_LIBS=TRUE
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --parallel "${DEPENDENCY_BUILD_JOBS}"
    BUILD_BYPRODUCTS "${_event_prefix}/lib/libevent_core.a"
    LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
)
set(MANAGED_COTURN_PREFIX "${CMAKE_BINARY_DIR}/_deps/coturn-4.6.3/install" CACHE INTERNAL "Managed coturn prefix")
# Upstream's native configure probes socket/platform features used by STUN.
# The 4.6.3 CMake build starts but fails the laboratory's STUN readiness probe.
check_deps(DEPENDENCIES patch)
set(_coturn_patch "${CMAKE_CURRENT_LIST_DIR}/../patches/coturn/0001-respect-tmpdir.patch")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_coturn_patch}")
file(SHA256 "${_coturn_patch}" _coturn_patch_hash)
string(SUBSTRING "${_coturn_patch_hash}" 0 12 _coturn_patch_id)
set(_coturn_ssl_libs "${MANAGED_OPENSSL_PREFIX}/lib/libssl.a ${MANAGED_OPENSSL_PREFIX}/lib/libcrypto.a -ldl -pthread")
set(_coturn_event_libs "${_event_prefix}/lib/libevent_openssl.a ${_event_prefix}/lib/libevent_pthreads.a ${_event_prefix}/lib/libevent_extra.a ${_event_prefix}/lib/libevent_core.a ${_coturn_ssl_libs}")
ExternalProject_Add(coturn_external
    PREFIX "${CMAKE_BINARY_DIR}/_deps/coturn-4.6.3-native-${_coturn_patch_id}"
    URL "https://codeload.github.com/coturn/coturn/tar.gz/refs/tags/4.6.3"
    URL_HASH SHA256=dc3a529fd9956dc8771752a7169c5ad4c18b9deef3ec96049de30fabf1637704
    DOWNLOAD_NAME coturn-4.6.3.tar.gz
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads" TLS_VERIFY TRUE
    DEPENDS openssl_external libevent_external
    PATCH_COMMAND "${PATCH_EXECUTABLE}" --batch --forward -p1 -i "${_coturn_patch}"
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env "CC=${CMAKE_C_COMPILER}"
        "TMPDIR=<SOURCE_DIR>" "CFLAGS=-O2" "SSL_CFLAGS=-I${MANAGED_OPENSSL_PREFIX}/include"
        "SSL_LIBS=${_coturn_ssl_libs}" "EVENT_CFLAGS=-I${_event_prefix}/include"
        "EVENT_LIBS=${_coturn_event_libs}"
        TURN_NO_SQLITE=1 TURN_NO_PQ=1 TURN_NO_MYSQL=1 TURN_NO_MONGO=1
        TURN_NO_HIREDIS=1 TURN_NO_SYSTEMD=1 TURN_NO_PROMETHEUS=1 TURN_NO_SCTP=1
        <SOURCE_DIR>/configure "--prefix=${MANAGED_COTURN_PREFIX}"
    BUILD_COMMAND "${MAKE_EXECUTABLE}" "-j${DEPENDENCY_BUILD_JOBS}" bin/turnserver
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E make_directory "${MANAGED_COTURN_PREFIX}/bin"
        COMMAND "${CMAKE_COMMAND}" -E copy <SOURCE_DIR>/bin/turnserver "${MANAGED_COTURN_PREFIX}/bin/turnserver"
    BUILD_BYPRODUCTS "${MANAGED_COTURN_PREFIX}/bin/turnserver"
    LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
)
add_executable(Coturn::turnserver IMPORTED GLOBAL)
set_target_properties(Coturn::turnserver PROPERTIES IMPORTED_LOCATION "${MANAGED_COTURN_PREFIX}/bin/turnserver")
add_dependencies(Coturn::turnserver coturn_external)
