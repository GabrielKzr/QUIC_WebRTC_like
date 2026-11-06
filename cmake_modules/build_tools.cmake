include_guard(GLOBAL)
include(ExternalProject)

# Bootstrap missing Python tools at build time, without pip or user installs.
set(MANAGED_BUILD_TOOLS_BIN "${CMAKE_BINARY_DIR}/_deps/build-tools/bin")
file(MAKE_DIRECTORY "${MANAGED_BUILD_TOOLS_BIN}")
find_program(MESON_EXECUTABLE NAMES meson)
find_program(GLIB_MKENUMS_EXECUTABLE NAMES glib-mkenums)
set(LIBNICE_TOOL_TARGETS)
if(NOT MESON_EXECUTABLE OR MESON_EXECUTABLE STREQUAL "${MANAGED_BUILD_TOOLS_BIN}/meson")
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    check_deps(DEPENDENCIES chmod)
    set(_meson_source "${CMAKE_BINARY_DIR}/_deps/meson-1.12.1/src/meson_external")
    set(MESON_EXECUTABLE "${MANAGED_BUILD_TOOLS_BIN}/meson")
    file(WRITE "${MESON_EXECUTABLE}"
        "#!${Python3_EXECUTABLE}\nimport sys\nsys.path.insert(0, r\"\"\"${_meson_source}\"\"\")\nfrom mesonbuild.mesonmain import main\nsys.exit(main())\n")
    execute_process(COMMAND "${CHMOD_EXECUTABLE}" +x "${MESON_EXECUTABLE}" RESULT_VARIABLE _chmod)
    if(NOT _chmod EQUAL 0)
        message(FATAL_ERROR "Cannot make managed Meson launcher executable")
    endif()
    ExternalProject_Add(meson_external
        PREFIX "${CMAKE_BINARY_DIR}/_deps/meson-1.12.1"
        URL "https://files.pythonhosted.org/packages/57/99/36cd25f9598eef70db97577440a70a360af505f6f01c2338a18d1556079c/meson-1.12.1-py3-none-any.whl"
        URL_HASH SHA256=930bc7542cbd9f57009e182fd014eba48cf1a0180a7b9006d2d32d8f168d8b02
        DOWNLOAD_NAME meson-1.12.1.zip
        DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads" TLS_VERIFY TRUE
        CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${_meson_source}/mesonbuild/mesonmain.py"
        LOG_DOWNLOAD ON LOG_OUTPUT_ON_FAILURE ON
    )
    list(APPEND LIBNICE_TOOL_TARGETS meson_external)
    message(STATUS "Meson absent: preparing managed Meson 1.12.1 during build")
endif()
if(NOT GLIB_MKENUMS_EXECUTABLE OR GLIB_MKENUMS_EXECUTABLE STREQUAL "${MANAGED_BUILD_TOOLS_BIN}/glib-mkenums")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    check_deps(DEPENDENCIES chmod)
    set(GLIB_MKENUMS_EXECUTABLE "${MANAGED_BUILD_TOOLS_BIN}/glib-mkenums")
    ExternalProject_Add(glib_mkenums_external
        PREFIX "${CMAKE_BINARY_DIR}/_deps/glib-mkenums-2.56.4"
        URL "https://raw.githubusercontent.com/GNOME/glib/2.56.4/gobject/glib-mkenums.in"
        URL_HASH SHA256=15944a32303d0c054a4063255c5e2cd4178f36f12c547009fc62b9d436c96727
        DOWNLOAD_NAME glib-mkenums-2.56.4.in DOWNLOAD_NO_EXTRACT TRUE
        DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/downloads" TLS_VERIFY TRUE
        CONFIGURE_COMMAND "" BUILD_COMMAND ""
        INSTALL_COMMAND "${CMAKE_COMMAND}"
            "-DINPUT=<DOWNLOADED_FILE>" "-DOUTPUT=${GLIB_MKENUMS_EXECUTABLE}"
            "-DPYTHON=${Python3_EXECUTABLE}" "-DCHMOD=${CHMOD_EXECUTABLE}"
            -P "${CMAKE_CURRENT_LIST_DIR}/prepare_glib_mkenums.cmake"
        BUILD_BYPRODUCTS "${GLIB_MKENUMS_EXECUTABLE}"
        LOG_DOWNLOAD ON LOG_OUTPUT_ON_FAILURE ON
    )
    list(APPEND LIBNICE_TOOL_TARGETS glib_mkenums_external)
    message(STATUS "glib-mkenums absent: preparing standalone GLib 2.56.4 tool during build")
endif()
