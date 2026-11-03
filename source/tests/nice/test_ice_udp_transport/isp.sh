#!/bin/sh
set -eu

command -v turnserver >/dev/null || {
    echo "turnserver não encontrado; este teste requer coturn no PATH" >&2
    exit 1
}
command -v python3 >/dev/null

script_dir=$(dirname "$0")

turnserver -n \
    --stun-only \
    --listening-port=3478 \
    --no-tcp \
    --no-tls \
    --no-dtls \
    --no-cli \
    --log-file=stdout \
    --pidfile=/dev/null &
stun_pid=$!

python3 -u "$script_dir/signaling.py" &
signaling_pid=$!

while kill -0 "$stun_pid" 2>/dev/null &&
      kill -0 "$signaling_pid" 2>/dev/null; do
    sleep 0.1
done

exit 1
