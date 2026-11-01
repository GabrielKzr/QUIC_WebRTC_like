find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNICE REQUIRED IMPORTED_TARGET nice)

set(LIB_EXTRA_LIBS udp_transport PkgConfig::LIBNICE)
