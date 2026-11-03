# WebRTC/test_chownat/test_chownat.meta.cmake

set(TEST_EXTRA_LIBS  chownat direct_udp_transport)

set(PATCHBAY_NAT_EXTRA_RULES
    "flush map ip nat fullcone"
    "add set ip nat opened_ports { type inet_service\; flags dynamic,timeout\; timeout 300s\; }"
    "add chain ip nat inbound_guard { type filter hook prerouting priority raw\; policy accept\; }"
    "add rule ip nat inbound_guard iif \"@WAN@\" meta l4proto udp udp dport != @opened_ports drop"
    "flush chain ip nat postrouting"
    "add rule ip nat postrouting oif \"@WAN@\" meta l4proto udp update @fullcone { udp sport timeout 300s : ip saddr . udp sport }"
    "add rule ip nat postrouting oif \"@WAN@\" meta l4proto udp update @opened_ports { udp sport timeout 300s }"
    "add rule ip nat postrouting oif \"@WAN@\" meta l4proto udp snat to @PUBLIC_IP@:udp sport"
)

set_target_properties(test_chownat PROPERTIES
    PATCHBAY_MODE tcp
    PATCHBAY_TIMEOUT 75
    PATCHBAY_TEST_NAME network_chownat_tcp
    PATCHBAY_LABELS "network;chownat"
)
