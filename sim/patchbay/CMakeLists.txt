if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Patchbay requires Linux")
endif()
find_program(CARGO_EXECUTABLE cargo HINTS "$ENV{HOME}/.cargo/bin")
find_program(NFT_EXECUTABLE nft)
find_program(TC_EXECUTABLE tc)
find_program(STDBUF_EXECUTABLE stdbuf)
find_program(TCPDUMP_EXECUTABLE tcpdump)
find_program(IP_EXECUTABLE ip)
foreach(tool IN ITEMS CARGO_EXECUTABLE NFT_EXECUTABLE TC_EXECUTABLE STDBUF_EXECUTABLE TCPDUMP_EXECUTABLE IP_EXECUTABLE)
    if(NOT ${tool})
        message(FATAL_ERROR "Missing dependency ${tool}; see sim/patchbay/README.md")
    endif()
endforeach()
set(PATCHBAY_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo")
set(PATCHBAY_RUNNER "${PATCHBAY_TARGET_DIR}/debug/quic-network-regression")
add_custom_target(patchbay_runner ALL
    COMMAND "${CMAKE_COMMAND}" -E env --unset=MAKEFLAGS --unset=MFLAGS "${CARGO_EXECUTABLE}" build --locked
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
        --target-dir "${PATCHBAY_TARGET_DIR}"
    BYPRODUCTS "${PATCHBAY_RUNNER}"
    USES_TERMINAL VERBATIM)
foreach(protocol IN ITEMS chownat openssl_quic)
    foreach(scenario IN ITEMS public nat)
        set(name "network_${protocol}_${scenario}")
        add_test(NAME "${name}" COMMAND "${PATCHBAY_RUNNER}"
            "$<TARGET_FILE:test_${protocol}>" "${scenario}"
            "${PROJECT_SOURCE_DIR}/source"
            "${CMAKE_CURRENT_BINARY_DIR}/artifacts/${name}")
        set_tests_properties("${name}" PROPERTIES LABELS "network;${protocol}"
            TIMEOUT 75 RUN_SERIAL TRUE)
    endforeach()
endforeach()

add_test(NAME network_environment_nat COMMAND "${PATCHBAY_RUNNER}"
    "$<TARGET_FILE:test_chownat>" smoke-nat "${PROJECT_SOURCE_DIR}/source"
    "${CMAKE_CURRENT_BINARY_DIR}/artifacts/network_environment_nat")
set_tests_properties(network_environment_nat PROPERTIES LABELS "network;environment"
    TIMEOUT 20 RUN_SERIAL TRUE)

foreach(scenario IN ITEMS gns3 udp-gns3 udp-nat)
    if(scenario STREQUAL "gns3")
        set(peer test_chownat)
    else()
        set(peer test_chownat_udp)
    endif()
    add_test(NAME "network_chownat_${scenario}" COMMAND "${PATCHBAY_RUNNER}"
        "$<TARGET_FILE:${peer}>" "${scenario}" "${PROJECT_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_BINARY_DIR}/artifacts/network_chownat_${scenario}")
    set_tests_properties("network_chownat_${scenario}" PROPERTIES
        LABELS "network;chownat;baseline" TIMEOUT 20 RUN_SERIAL TRUE)
endforeach()
