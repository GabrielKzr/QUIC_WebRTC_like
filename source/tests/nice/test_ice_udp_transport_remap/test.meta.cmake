set(TEST_EXTRA_LIBS ice_udp_transport)
set(TEST_EXTRA_DEFINES ICE_REQUIRE_PORT_REMAP)

if(BUILD_TESTING AND BUILD_NETWORK_TESTS)
    check_deps(
        DEPENDENCIES turnserver python3
        HELP_MESSAGE "CMake builds coturn automatically; Python 3 must be available for signaling. See docs/build-dependencies.md"
    )
endif()

# O XOR troca a porta de origem e indexa o DNAT pela porta publica resultante.
set(PATCHBAY_NAT_EXTRA_RULES
    "flush map ip nat fullcone"
    "flush chain ip nat postrouting"
    "add rule ip nat postrouting oif \"@WAN@\" meta l4proto udp update @fullcone { udp sport ^ 0x4000 timeout 300s : ip saddr . udp sport }"
    "add rule ip nat postrouting oif \"@WAN@\" meta l4proto udp snat to @PUBLIC_IP@:(udp sport ^ 0x4000)"
    "add rule ip nat postrouting oif \"@WAN@\" ip daddr @ISP_IP@ meta l4proto tcp snat to @PUBLIC_IP@"
)

set_target_properties(test_ice_udp_transport_remap PROPERTIES
    PATCHBAY_MODE udp
    PATCHBAY_TIMEOUT 20
    PATCHBAY_TEST_NAME network_ice_udp_transport_remap
    PATCHBAY_LABELS "network;nice;ice;stun;remap"
    PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../test_ice_udp_transport/isp.sh"
)
