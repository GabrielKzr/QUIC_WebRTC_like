include_guard(GLOBAL)
include(ExternalProject)
check_deps(DEPENDENCIES flex bison)
set(_pcap_prefix "${CMAKE_BINARY_DIR}/_deps/libpcap-1.10.5/install")
ExternalProject_Add(libpcap_external
    PREFIX "${CMAKE_BINARY_DIR}/_deps/libpcap-1.10.5"
    URL "https://www.tcpdump.org/release/libpcap-1.10.5.tar.gz"
    URL_HASH SHA256=37ced90a19a302a7f32e458224a00c365c117905c2cd35ac544b6880a81488f0
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads" TLS_VERIFY TRUE
    CMAKE_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" -DCMAKE_BUILD_TYPE=Release
        "-DCMAKE_INSTALL_PREFIX=${_pcap_prefix}" -DCMAKE_INSTALL_LIBDIR=lib
        -DBUILD_SHARED_LIBS=OFF -DDISABLE_DBUS=ON -DDISABLE_RDMA=ON
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --parallel "${DEPENDENCY_BUILD_JOBS}"
    BUILD_BYPRODUCTS "${_pcap_prefix}/lib/libpcap.a"
    LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
)
set(MANAGED_PCAP_PREFIX "${CMAKE_BINARY_DIR}/_deps/tcpdump-4.99.5/install" CACHE INTERNAL "Managed tcpdump prefix")
ExternalProject_Add(tcpdump_external
    PREFIX "${CMAKE_BINARY_DIR}/_deps/tcpdump-4.99.5"
    URL "https://codeload.github.com/the-tcpdump-group/tcpdump/tar.gz/refs/tags/tcpdump-4.99.5"
    URL_HASH SHA256=7f5dde921c3234c06b89543a999a1991fe89ba5e37b91edae3eb96c858c6bc1a
    DOWNLOAD_NAME tcpdump-4.99.5.tar.gz
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads" TLS_VERIFY TRUE
    DEPENDS libpcap_external
    CMAKE_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" -DCMAKE_BUILD_TYPE=Release
        "-DCMAKE_INSTALL_PREFIX=${MANAGED_PCAP_PREFIX}"
        "-DCMAKE_PREFIX_PATH=${_pcap_prefix}" "-U*CONFIG_PCAP*"
        -DWITH_USER= -DWITH_CAP_NG=OFF -DWITH_CAPSICUM=OFF -DWITH_CRYPTO=OFF -DWITH_SMI=OFF
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --parallel "${DEPENDENCY_BUILD_JOBS}"
    BUILD_BYPRODUCTS "${MANAGED_PCAP_PREFIX}/bin/tcpdump"
    LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
)
