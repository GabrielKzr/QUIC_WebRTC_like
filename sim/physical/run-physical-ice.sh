#!/usr/bin/env bash
# Experimento físico QUIC/ICE direto: execute com papel a ou b em cada PC.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_dir"
artifact_root="$repo_dir/sim/physical/artifacts"
role=${1:-}
if [[ $role != a && $role != b ]]; then
    echo "Uso: sudo bash sim/physical/run-physical-ice.sh a (PC 1)" >&2
    echo "  ou bash sim/physical/run-physical-ice.sh b (PC 2)" >&2
    exit 1
fi

peer_bin="$repo_dir/build/all/source/tests/test_quic_ice"
[[ -x "$peer_bin" ]] || {
    echo "Binário ausente. Rode cmake --preset all e cmake --build --preset all --target test_quic_ice -j2" >&2
    exit 1
}
command -v tcpdump >/dev/null || {
    echo "tcpdump não encontrado; instale-o para gerar PCAPs" >&2
    exit 1
}

if [[ $role == b ]]; then
    if (( EUID != 0 )); then
        echo "Execute: sudo bash sim/physical/run-physical-ice.sh b" >&2
        exit 1
    fi
    ip -br -4 addr | grep -q '192\.168\.80\.101/' || {
        echo "PC 2 não tem 192.168.80.101; abortando" >&2
        exit 1
    }
    log_dir="$artifact_root/b-$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$log_dir"
    peer_b_interface=$(ip -o -4 addr show | awk '$4 ~ /^192\.168\.80\.101\// { print $2; exit }')
    echo_pid=
    capture_b_pid=
    cleanup_b() {
        set +e
        if [[ -n $capture_b_pid ]]; then
            kill -INT "$capture_b_pid" 2>/dev/null
            wait "$capture_b_pid" 2>/dev/null
        fi
        if [[ -n $echo_pid ]]; then
            kill "$echo_pid" 2>/dev/null
            wait "$echo_pid" 2>/dev/null
        fi
        if [[ -f $log_dir/peer-b-lan.pcap ]]; then
            echo "PCAP peer B: $log_dir/peer-b-lan.pcap ($(tcpdump -nnr "$log_dir/peer-b-lan.pcap" 2>/dev/null | wc -l) pacotes)"
        fi
        if [[ -n ${SUDO_UID:-} && -n ${SUDO_GID:-} ]]; then
            chown "$SUDO_UID:$SUDO_GID" "$artifact_root"
            chown -R "$SUDO_UID:$SUDO_GID" "$log_dir"
        fi
        echo "Logs em $log_dir"
    }
    trap cleanup_b EXIT
    trap 'exit 129' HUP
    trap 'exit 130' INT
    trap 'exit 143' TERM
    tcpdump -i "$peer_b_interface" -nn -s 0 -U -Z root \
        -w "$log_dir/peer-b-lan.pcap" 'udp or tcp port 5555' \
        >"$log_dir/peer-b-tcpdump.log" 2>&1 &
    capture_b_pid=$!
    sleep 0.2
    kill -0 "$capture_b_pid" 2>/dev/null || {
        echo "Captura do peer B falhou; veja $log_dir/peer-b-tcpdump.log" >&2
        exit 1
    }
    python3 -u - >"$log_dir/echo.log" 2>&1 <<'PY' &
import socket
with socket.socket() as listener:
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(("127.0.0.1", 1234))
    listener.listen()
    print("Echo pronto", flush=True)
    conn, _ = listener.accept()
    with conn:
        while data := conn.recv(8192):
            conn.sendall(data)
print("Echo encerrado", flush=True)
PY
    echo_pid=$!
    ready=0
    for _ in {1..30}; do
        kill -0 "$echo_pid" 2>/dev/null || break
        if grep -q '^Echo pronto$' "$log_dir/echo.log"; then ready=1; break; fi
        sleep 0.1
    done
    if (( ! ready )); then
        echo "Echo não ficou pronto; veja $log_dir/echo.log" >&2
        exit 1
    fi
    echo "Iniciando peer B contra signaling 10.0.0.108:5555..."
    cd "$repo_dir/source"
    env PATCHBAY_ISP_ADDRESS=10.0.0.108 PATCHBAY_LOCAL_ADDRESS=192.168.80.101 \
        "$peer_bin" -d -s 1234 127.0.0.1 2>&1 | tee "$log_dir/peer-b.log"
    exit "${PIPESTATUS[0]}"
fi

if (( EUID != 0 )); then
    echo "Execute: sudo bash sim/physical/run-physical-ice.sh a" >&2
    exit 1
fi

namespace=physical-peer-a
ethernet=enp0s31f6
server_ip=10.0.0.108
peer_ip=192.168.80.100
peer_gateway=192.168.80.1
log_dir="$artifact_root/a-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$log_dir"

turn_bin="$repo_dir/build/all/_deps/coturn-4.6.3/install/bin/turnserver"
[[ -x "$turn_bin" ]] || {
    echo "Binários ausentes. Rode cmake --preset all e cmake --build --preset all --target test_quic_ice -j2" >&2
    exit 1
}

# Recupera uma execução anterior interrompida depois de mover a Ethernet.
if ! ip link show "$ethernet" >/dev/null 2>&1 &&
   ip netns list | grep -q "^$namespace "; then
    if [[ -n $(ip netns pids "$namespace") ]]; then
        echo "Namespace $namespace ainda tem processos; abortando" >&2
        exit 1
    fi
    if ! ip -n "$namespace" link show "$ethernet" >/dev/null 2>&1; then
        echo "$ethernet ausente também em $namespace; abortando" >&2
        exit 1
    fi
    echo "Recuperando $ethernet do namespace deixado pela execução anterior..."
    ip netns exec "$namespace" ip link set "$ethernet" netns 1
    ip netns del "$namespace"
    ip link set "$ethernet" up
    ip addr replace "$peer_ip/24" dev "$ethernet"
    if command -v nmcli >/dev/null; then nmcli device connect "$ethernet" || true; fi
fi

ip -br -4 addr show "$ethernet" | grep -q "$peer_ip/" || {
    echo "$ethernet não tem $peer_ip; abortando" >&2
    exit 1
}
ip -br -4 addr show wlp0s20f3 | grep -q "$server_ip/" || {
    echo "Wi-Fi não tem $server_ip; abortando" >&2
    exit 1
}
[[ -z $(ip netns list) ]] || {
    echo "Já existe namespace de rede; confira antes de executar" >&2
    exit 1
}
[[ -z $(ss -H -ltn '( sport = :5555 )') && -z $(ss -H -lun '( sport = :3478 )') ]] || {
    echo "Porta 5555 ou 3478 ocupada; encerre serviços anteriores" >&2
    exit 1
}

turn_pid=
signaling_pid=
probe_pid=
server_capture_pid=
peer_capture_pid=
namespace_created=0
ethernet_moved=0
cleanup() {
    set +e
    [[ -z $probe_pid ]] || { kill "$probe_pid" 2>/dev/null; wait "$probe_pid" 2>/dev/null; }
    if [[ -n $peer_capture_pid ]]; then
        kill -INT "$peer_capture_pid" 2>/dev/null
        wait "$peer_capture_pid" 2>/dev/null
    fi
    if [[ -n $server_capture_pid ]]; then
        kill -INT "$server_capture_pid" 2>/dev/null
        wait "$server_capture_pid" 2>/dev/null
    fi
    [[ -z $signaling_pid ]] || { kill "$signaling_pid" 2>/dev/null; wait "$signaling_pid" 2>/dev/null; }
    [[ -z $turn_pid ]] || { kill "$turn_pid" 2>/dev/null; wait "$turn_pid" 2>/dev/null; }
    if (( ethernet_moved )); then
        ip netns exec "$namespace" ip link set "$ethernet" netns 1
        ip link set "$ethernet" up
        ip addr replace "$peer_ip/24" dev "$ethernet"
        ip route replace default via "$peer_gateway" dev "$ethernet" metric 100
        if command -v nmcli >/dev/null; then nmcli device connect "$ethernet"; fi
    fi
    if (( namespace_created )); then ip netns del "$namespace"; fi
    for pcap in "$log_dir/server-wifi.pcap" "$log_dir/peer-a-lan.pcap"; do
        if [[ -f $pcap ]]; then
            echo "PCAP: $pcap ($(tcpdump -nnr "$pcap" 2>/dev/null | wc -l) pacotes)"
        fi
    done
    if [[ -n ${SUDO_UID:-} && -n ${SUDO_GID:-} ]]; then
        chown "$SUDO_UID:$SUDO_GID" "$artifact_root"
        chown -R "$SUDO_UID:$SUDO_GID" "$log_dir"
    fi
    echo "Logs em $log_dir"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

tcpdump -i wlp0s20f3 -nn -s 0 -U -Z root \
    -w "$log_dir/server-wifi.pcap" 'udp or tcp port 5555' \
    >"$log_dir/server-tcpdump.log" 2>&1 &
server_capture_pid=$!
sleep 0.2
kill -0 "$server_capture_pid" 2>/dev/null || {
    echo "Captura do servidor falhou; veja $log_dir/server-tcpdump.log" >&2
    exit 1
}

"$turn_bin" -n \
    --listening-ip="$server_ip" --listening-ip=127.0.0.1 \
    --relay-ip="$server_ip" --listening-port=3478 \
    --min-port=49160 --max-port=49200 \
    --lt-cred-mech --realm=ice-test --user=ice-test:ice-test-password \
    --no-tcp --no-tls --no-dtls --no-cli \
    --log-file=stdout --pidfile=/dev/null >"$log_dir/turn.log" 2>&1 &
turn_pid=$!

python3 -u source/tests/nice/test_ice_udp_transport/signaling.py \
    >"$log_dir/signaling.log" 2>&1 &
signaling_pid=$!

ready=0
for _ in {1..50}; do
    kill -0 "$turn_pid" 2>/dev/null || break
    kill -0 "$signaling_pid" 2>/dev/null || break
    if grep -q '^READY$' "$log_dir/signaling.log"; then ready=1; break; fi
    sleep 0.1
done
if (( ! ready )); then
    echo "Signaling não ficou pronto; veja $log_dir/signaling.log" >&2
    exit 1
fi

ip netns add "$namespace"
namespace_created=1
ip link set "$ethernet" netns "$namespace"
ethernet_moved=1
ip -n "$namespace" link set lo up
ip -n "$namespace" addr replace "$peer_ip/24" dev "$ethernet"
ip -n "$namespace" link set "$ethernet" up
ip -n "$namespace" route replace default via "$peer_gateway"
ip netns exec "$namespace" ip route get "$server_ip" | tee "$log_dir/route.txt"

ip netns exec "$namespace" tcpdump -i "$ethernet" -nn -s 0 -U -Z root \
    -w "$log_dir/peer-a-lan.pcap" 'udp or tcp port 5555' \
    >"$log_dir/peer-a-tcpdump.log" 2>&1 &
peer_capture_pid=$!
sleep 0.2
kill -0 "$peer_capture_pid" 2>/dev/null || {
    echo "Captura do peer A falhou; veja $log_dir/peer-a-tcpdump.log" >&2
    exit 1
}

ip netns exec "$namespace" python3 -u - >"$log_dir/probe.log" 2>&1 <<'PY' &
import socket
import time

deadline = time.monotonic() + 120
while True:
    try:
        conn = socket.create_connection(("127.0.0.1", 44444), timeout=1)
        break
    except OSError:
        if time.monotonic() >= deadline:
            raise
        time.sleep(0.05)
with conn:
    conn.settimeout(20)
    payload = bytes(n % 251 for n in range(16384))
    conn.sendall(payload)
    received = bytearray()
    while len(received) < len(payload):
        block = conn.recv(len(payload) - len(received))
        if not block:
            raise RuntimeError("EOF antecipado")
        received.extend(block)
    assert received == payload, "Dados diferentes"
    conn.shutdown(socket.SHUT_WR)
    assert conn.recv(1) == b"", "Dados extras"
print("OK: 16384 bytes íntegros e EOF", flush=True)
PY
probe_pid=$!

echo "STUN/signaling prontos. Inicie agora o test_quic_ice do peer B no PC 2."
echo "Aguardando o PC 2 conectar ao signaling por até 120 segundos..."
connected=0
for _ in {1..1200}; do
    kill -0 "$signaling_pid" 2>/dev/null || break
    if [[ -n $(ss -H -tn state established '( sport = :5555 )') ]]; then
        connected=1
        break
    fi
    sleep 0.1
done
if (( ! connected )); then
    echo "Peer B não conectou ao signaling; veja $log_dir" >&2
    exit 1
fi

echo "Peer B conectou. Iniciando peer A..."
set +e
(
    cd "$repo_dir/source"
    ip netns exec "$namespace" env \
        PATCHBAY_ISP_ADDRESS="$server_ip" \
        PATCHBAY_LOCAL_ADDRESS="$peer_ip" \
        "$peer_bin" -d -c 44444 127.0.0.1
) 2>&1 | tee "$log_dir/peer-a.log"
peer_status=${PIPESTATUS[0]}
if (( peer_status != 0 )); then
    echo "Peer A falhou; veja $log_dir/peer-a.log e a saída do PC 2" >&2
    exit "$peer_status"
fi
wait "$probe_pid"
probe_status=$?
probe_pid=
cat "$log_dir/probe.log"
exit "$probe_status"
