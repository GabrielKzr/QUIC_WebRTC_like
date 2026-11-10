#!/usr/bin/env bash
# Experimento físico QUIC/ICE direto: execute com papel a ou b em cada PC.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_dir"
artifact_root="$repo_dir/sim/physical/artifacts"
role=${1:-}
if [[ $role != a && $role != b && $role != cleanup && $role != stop ]]; then
    echo "Uso: sudo bash sim/physical/run-physical-ice.sh a (PC 1)" >&2
    echo "  ou sudo bash sim/physical/run-physical-ice.sh b (PC 2)" >&2
    echo "  ou sudo bash sim/physical/run-physical-ice.sh cleanup (recuperar PC 1)" >&2
    echo "  ou sudo bash sim/physical/run-physical-ice.sh stop (liberar captura presa)" >&2
    exit 1
fi
if [[ $role == stop ]]; then
    if (( EUID != 0 )); then
        echo "Execute: sudo bash sim/physical/run-physical-ice.sh stop" >&2
        exit 1
    fi
    while read -r old_script_pid; do
        [[ -n $old_script_pid && $old_script_pid != "$$" ]] || continue
        mapfile -t child_pids < <(pgrep -P "$old_script_pid" || true)
        if (( ${#child_pids[@]} )); then
            kill -KILL "${child_pids[@]}" 2>/dev/null || true
        fi
        kill -KILL "$old_script_pid" 2>/dev/null || true
    done < <(pgrep -f '^bash sim/physical/run-physical-ice.sh a$' || true)
    while read -r capture_pid; do
        [[ -n $capture_pid ]] || continue
        capture_command=$(tr '\0' ' ' <"/proc/$capture_pid/cmdline" 2>/dev/null || true)
        [[ $capture_command == *"$artifact_root/a-"* ]] || continue
        echo "Parando captura física PID $capture_pid..."
        kill -KILL "$capture_pid" 2>/dev/null || true
    done < <(pgrep -x tcpdump || true)
    sleep 0.2
    if ip netns list | grep -q '^physical-peer-a '; then
        while read -r namespace_pid; do
            [[ -n $namespace_pid ]] && kill -KILL "$namespace_pid" 2>/dev/null || true
        done < <(ip netns pids physical-peer-a)
        if ip -n physical-peer-a link show enp0s31f6 >/dev/null 2>&1; then
            ip netns exec physical-peer-a ip link set enp0s31f6 netns 1
        fi
        ip netns del physical-peer-a
    fi
    if ip link show enp0s31f6 >/dev/null 2>&1; then
        ip link set enp0s31f6 up
        ip addr replace 192.168.80.100/24 dev enp0s31f6
        ip route replace default via 192.168.80.1 dev enp0s31f6 metric 100
        if command -v nmcli >/dev/null; then timeout 10s nmcli device connect enp0s31f6 || true; fi
    fi
    echo "Execução física anterior encerrada."
    exit 0
fi
cleanup_only=0
if [[ $role == cleanup ]]; then
    cleanup_only=1
    role=a
fi

stop_process() {
    [[ -n $1 ]] || return 0
    kill -KILL "$1" 2>/dev/null || true
}

peer_bin="$repo_dir/build/all/source/tests/test_quic_ice"
[[ -x "$peer_bin" ]] || {
    echo "Binário ausente. Rode cmake --preset all e cmake --build --preset all --target test_quic_ice -j2" >&2
    exit 1
}
command -v tcpdump >/dev/null || {
    echo "tcpdump não encontrado; instale-o para gerar PCAPs" >&2
    exit 1
}
command -v nsenter >/dev/null || {
    echo "nsenter não encontrado; instale util-linux para capturar o peer A" >&2
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
        trap - EXIT
        trap '' HUP INT TERM
        sleep 1
        stop_process "$capture_b_pid"
        stop_process "$echo_pid"
        echo "PCAP peer B: $log_dir/peer-b-lan.pcap"
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
if (( ! cleanup_only )); then mkdir -p "$log_dir"; fi

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

# A interface pode já ter voltado ao host, mas o nome do namespace ter sobrado.
if ip link show "$ethernet" >/dev/null 2>&1 &&
   ip netns list | grep -q "^$namespace "; then
    if [[ -n $(ip netns pids "$namespace") ]]; then
        echo "Namespace $namespace ainda tem processos; abortando" >&2
        exit 1
    fi
    ip netns del "$namespace"
    echo "Namespace vazio $namespace removido."
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

# Uma sessão interrompida pode deixar o signaling anterior escutando.
# Encerra apenas processos cuja saída aponta para artefatos deste script.
stale_signaling_found=0
while read -r stale_pid; do
    [[ -n $stale_pid ]] || continue
    stale_stdout=$(readlink -f "/proc/$stale_pid/fd/1" 2>/dev/null || true)
    if [[ $stale_stdout == "$artifact_root"/a-*/signaling.log ]]; then
        echo "Encerrando signaling antigo da execução física (PID $stale_pid)..."
        kill "$stale_pid" 2>/dev/null || true
        stale_signaling_found=1
    fi
done < <(pgrep -f '^python3 -u source/tests/nice/test_ice_udp_transport/signaling.py$' || true)
if (( stale_signaling_found )); then
    for _ in {1..20}; do
        [[ -n $(ss -H -ltn '( sport = :5555 )') ]] || break
        sleep 0.1
    done
fi

if (( cleanup_only )); then
    echo "Recuperação concluída. Estado atual:"
    ip -br -4 addr show "$ethernet"
    ss -H -ltn '( sport = :5555 )'
    ss -H -lun '( sport = :3478 )'
    exit 0
fi

[[ -z $(ss -H -ltn '( sport = :5555 )') && -z $(ss -H -lun '( sport = :3478 )') ]] || {
    echo "Porta 5555 ou 3478 ocupada por outro processo:" >&2
    ss -ltnp '( sport = :5555 )' >&2
    ss -lunp '( sport = :3478 )' >&2
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
    trap - EXIT
    trap '' HUP INT TERM
    echo "Encerrando capturas e serviços..."
    # Dá tempo para tcpdump esvaziar a fila do kernel antes de remover a interface.
    sleep 1
    echo "Parando capturador do peer A..."
    stop_process "$peer_capture_pid"
    echo "Parando capturador do servidor..."
    stop_process "$server_capture_pid"
    echo "Parando probe e serviços..."
    stop_process "$probe_pid"
    stop_process "$signaling_pid"
    stop_process "$turn_pid"
    sleep 0.2
    if (( ethernet_moved )); then
        ip netns exec "$namespace" ip link set "$ethernet" netns 1
        ip link set "$ethernet" up
        ip addr replace "$peer_ip/24" dev "$ethernet"
        ip route replace default via "$peer_gateway" dev "$ethernet" metric 100
        if command -v nmcli >/dev/null; then timeout 10s nmcli device connect "$ethernet" || true; fi
    fi
    if (( namespace_created )); then timeout 5s ip netns del "$namespace"; fi
    echo "PCAP servidor: $log_dir/server-wifi.pcap"
    echo "PCAP peer A: $log_dir/peer-a-lan.pcap"
    if [[ -n ${SUDO_UID:-} && -n ${SUDO_GID:-} ]]; then
        chown "$SUDO_UID:$SUDO_GID" "$artifact_root"
        chown -R "$SUDO_UID:$SUDO_GID" "$log_dir"
    fi
    echo "Logs em $log_dir"
    echo "Limpeza concluída."
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

# Evita a porta TCP local fixa de uma execução anterior ainda ativa.
local_tcp_port=$(ip netns exec "$namespace" python3 -c \
    'import socket; s=socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1]); s.close()')
echo "Porta TCP local do peer A: $local_tcp_port" | tee "$log_dir/local-tcp-port.txt"

nsenter --net="/var/run/netns/$namespace" -- \
    tcpdump -i "$ethernet" -nn -s 0 -U -Z root \
    -w "$log_dir/peer-a-lan.pcap" 'udp or tcp port 5555' \
    >"$log_dir/peer-a-tcpdump.log" 2>&1 &
peer_capture_pid=$!
sleep 0.2
kill -0 "$peer_capture_pid" 2>/dev/null || {
    echo "Captura do peer A falhou; veja $log_dir/peer-a-tcpdump.log" >&2
    exit 1
}

ip netns exec "$namespace" env LOCAL_TCP_PORT="$local_tcp_port" \
    python3 -u - >"$log_dir/probe.log" 2>&1 <<'PY' &
import socket
import time
import os

deadline = time.monotonic() + 120
while True:
    try:
        conn = socket.create_connection(("127.0.0.1", int(os.environ["LOCAL_TCP_PORT"])), timeout=1)
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
        "$peer_bin" -d -c "$local_tcp_port" 127.0.0.1
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
