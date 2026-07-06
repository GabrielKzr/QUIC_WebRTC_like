#!/bin/bash
# Simula duas redes atrás de NAT, ligadas por uma "internet" central.
# Uso: sudo ./netns_nat_lab.sh up   -> monta a topologia
#      sudo ./netns_nat_lab.sh down -> desfaz tudo
set -e

up() {
    ip netns add internet
    ip netns add gwA
    ip netns add gwB
    ip netns add client
    ip netns add server

    # internet <-> gwA
    ip link add veth-int-a type veth peer name veth-a-int
    ip link set veth-int-a netns internet
    ip link set veth-a-int netns gwA

    # internet <-> gwB
    ip link add veth-int-b type veth peer name veth-b-int
    ip link set veth-int-b netns internet
    ip link set veth-b-int netns gwB

    # gwA <-> client
    ip link add veth-a-cli type veth peer name veth-cli-a
    ip link set veth-a-cli netns gwA
    ip link set veth-cli-a netns client

    # gwB <-> server
    ip link add veth-b-srv type veth peer name veth-srv-b
    ip link set veth-b-srv netns gwB
    ip link set veth-srv-b netns server

    # internet
    ip netns exec internet ip addr add 10.0.0.1/24 dev veth-int-a
    ip netns exec internet ip addr add 10.0.1.1/24 dev veth-int-b
    ip netns exec internet ip link set veth-int-a up
    ip netns exec internet ip link set veth-int-b up
    ip netns exec internet ip link set lo up
    ip netns exec internet sysctl -qw net.ipv4.ip_forward=1

    # gwA (NAT do lado do client)
    ip netns exec gwA ip addr add 10.0.0.2/24 dev veth-a-int
    ip netns exec gwA ip addr add 192.168.10.1/24 dev veth-a-cli
    ip netns exec gwA ip link set veth-a-int up
    ip netns exec gwA ip link set veth-a-cli up
    ip netns exec gwA ip link set lo up
    ip netns exec gwA ip route add default via 10.0.0.1
    ip netns exec gwA sysctl -qw net.ipv4.ip_forward=1
    ip netns exec gwA iptables -t nat -A POSTROUTING -o veth-a-int -j MASQUERADE

    # gwB (NAT do lado do server)
    ip netns exec gwB ip addr add 10.0.1.2/24 dev veth-b-int
    ip netns exec gwB ip addr add 192.168.20.1/24 dev veth-b-srv
    ip netns exec gwB ip link set veth-b-int up
    ip netns exec gwB ip link set veth-b-srv up
    ip netns exec gwB ip link set lo up
    ip netns exec gwB ip route add default via 10.0.1.1
    ip netns exec gwB sysctl -qw net.ipv4.ip_forward=1
    ip netns exec gwB iptables -t nat -A POSTROUTING -o veth-b-int -j MASQUERADE

    # client
    ip netns exec client ip addr add 192.168.10.10/24 dev veth-cli-a
    ip netns exec client ip link set veth-cli-a up
    ip netns exec client ip link set lo up
    ip netns exec client ip route add default via 192.168.10.1

    # server
    ip netns exec server ip addr add 192.168.20.10/24 dev veth-srv-b
    ip netns exec server ip link set veth-srv-b up
    ip netns exec server ip link set lo up
    ip netns exec server ip route add default via 192.168.20.1

    echo "Topologia pronta."
    echo "Rode teu binário assim:"
    echo "  ip netns exec client <bin> -c ..."
    echo "  ip netns exec server <bin> -s ..."
}

down() {
    for ns in internet gwA gwB client server; do
        ip netns del "$ns" 2>/dev/null || true
    done
    echo "Desfeito."
}

case "$1" in
    up) up ;;
    down) down ;;
    *) echo "uso: $0 {up|down}"; exit 1 ;;
esac