include_guard(GLOBAL)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(ExternalProject)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNICE_GLIB REQUIRED IMPORTED_TARGET GLOBAL
    gio-2.0>=2.54 gthread-2.0)
pkg_check_modules(LIBNICE_CRYPTO REQUIRED IMPORTED_TARGET GLOBAL gnutls>=2.12)
check_deps(DEPENDENCIES meson ninja patch
    HELP_MESSAGE "Building patched libnice requires Meson, Ninja and patch. See patches/libnice/README.md")

set(_libnice_patch "${CMAKE_CURRENT_LIST_DIR}/../patches/libnice/0001-retry-turn-deallocation-on-stale-nonce.patch")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_libnice_patch}")
file(SHA256 "${_libnice_patch}" _libnice_patch_hash)
string(SUBSTRING "${_libnice_patch_hash}" 0 12 _libnice_patch_id)
set(_libnice_root "${CMAKE_BINARY_DIR}/_deps/libnice-0.1.21-${_libnice_patch_id}")
set(_libnice_install "${_libnice_root}/install")

ExternalProject_Add(libnice_external
    PREFIX "${_libnice_root}"
    URL "https://codeload.github.com/libnice/libnice/tar.gz/refs/tags/0.1.21"
    URL_HASH SHA256=0f0d9a6b21cfb512593eacbba2a1981a3ba493341dd7377298085d47954b4ba4
    TLS_VERIFY TRUE
    DOWNLOAD_NAME libnice-0.1.21.tar.gz
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads"
    PATCH_COMMAND "${PATCH_EXECUTABLE}" --batch --forward -p1 -i "${_libnice_patch}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env "CC=${CMAKE_C_COMPILER}"
        "${MESON_EXECUTABLE}" setup <BINARY_DIR> <SOURCE_DIR>
        --prefix "${_libnice_install}" --libdir lib
        --buildtype debugoptimized --default-library static --wrap-mode nodownload
        -Db_staticpic=true -Dcrypto-library=gnutls
        -Dgupnp=disabled -Dgstreamer=disabled -Dintrospection=disabled
        -Dgtk_doc=disabled -Dexamples=disabled -Dtests=disabled
    BUILD_COMMAND "${NINJA_EXECUTABLE}" -C <BINARY_DIR>
    INSTALL_COMMAND "${NINJA_EXECUTABLE}" -C <BINARY_DIR> install
    BUILD_BYPRODUCTS "${_libnice_install}/lib/libnice.a"
)

# Os headers serão instalados pelo ExternalProject antes dos consumidores.
file(MAKE_DIRECTORY "${_libnice_install}/include/nice")
add_library(Libnice::Nice STATIC IMPORTED GLOBAL)
set_target_properties(Libnice::Nice PROPERTIES
    IMPORTED_LOCATION "${_libnice_install}/lib/libnice.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_libnice_install}/include;${_libnice_install}/include/nice"
    INTERFACE_LINK_LIBRARIES "PkgConfig::LIBNICE_GLIB;PkgConfig::LIBNICE_CRYPTO"
)
add_dependencies(Libnice::Nice libnice_external)
