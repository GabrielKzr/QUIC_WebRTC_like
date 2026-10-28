if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Patchbay requires Linux")
endif()

check_deps(
    DEPENDENCIES cargo nft tc stdbuf ip
    HINTS "$ENV{HOME}/.cargo/bin"
    HELP_MESSAGE "See sim/patchbay/README.md"
)

set(PATCHBAY_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo")
set(PATCHBAY_RUNNER "${PATCHBAY_TARGET_DIR}/debug/quic-network-regression")

add_custom_target(patchbay_runner ALL
    COMMAND "${CMAKE_COMMAND}" -E env --unset=MAKEFLAGS --unset=MFLAGS "${CARGO_EXECUTABLE}" build --locked
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
        --target-dir "${PATCHBAY_TARGET_DIR}"
    BYPRODUCTS "${PATCHBAY_RUNNER}"
    USES_TERMINAL VERBATIM)

# Target executavel para referenciar o runner sem exportar variaveis de caminho.
add_executable(patchbay_executor IMPORTED GLOBAL)
set_target_properties(patchbay_executor PROPERTIES IMPORTED_LOCATION "${PATCHBAY_RUNNER}")
add_dependencies(patchbay_executor patchbay_runner)

if(BUILD_TESTING)
    include("${CMAKE_CURRENT_SOURCE_DIR}/program_tests.cmake")
    register_program_tests(
        RUNNER "$<TARGET_FILE:patchbay_executor>"
        ARTIFACTS_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/artifacts"
    )
endif()
