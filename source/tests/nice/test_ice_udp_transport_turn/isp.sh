#!/bin/sh
set -eu

command -v turnserver >/dev/null || {
    echo "turnserver não encontrado; este teste requer coturn no PATH" >&2
    exit 1
}
command -v python3 >/dev/null

script_dir=$(dirname "$0")

# Credenciais exclusivas do laboratorio; nenhuma configuracao do host e usada.
turnserver -n \
    --verbose \
    --lt-cred-mech \
    --realm=ice-test \
    --user=ice-test:ice-test-password \
    --listening-port=3478 \
    --min-port=49160 \
    --max-port=49200 \
    --no-tcp \
    --no-tls \
    --no-dtls \
    --no-cli \
    --log-file=stdout \
    --pidfile=/dev/null &
turn_pid=$!

# O stdout e o arquivo isp.stdout.log criado pelo runner.
python3 -u "$script_dir/../test_ice_udp_transport/signaling.py" \
    --turn-log "/proc/$$/fd/1" &
signaling_pid=$!

while kill -0 "$turn_pid" 2>/dev/null &&
      kill -0 "$signaling_pid" 2>/dev/null; do
    sleep 0.1
done

exit 1
