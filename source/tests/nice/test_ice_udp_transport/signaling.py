import argparse
import os
import re
import socket
import struct
import time


def wait_stun():
    transaction = os.urandom(12)
    request = struct.pack("!HHI12s", 1, 0, 0x2112A442, transaction)
    deadline = time.monotonic() + 3

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
        probe.settimeout(0.1)

        while time.monotonic() < deadline:
            probe.sendto(request, ("127.0.0.1", 3478))
            try:
                response, _ = probe.recvfrom(2048)
            except socket.timeout:
                continue

            if (len(response) >= 20 and
                    response[:2] == b"\x01\x01" and
                    response[4:20] == request[4:20]):
                return

    raise RuntimeError("STUN não respondeu")


def read_message(stream):
    length = struct.unpack("!I", stream.read(4))[0]
    if not 0 < length <= 65536:
        raise ValueError("Tamanho inválido no signaling")

    message = stream.read(length)
    if len(message) != length:
        raise EOFError("Mensagem incompleta no signaling")

    return message


def wait_turn_release(path):
    deadline = time.monotonic() + 2
    pattern = re.compile(
        r"session (\d+): (new|refreshed), realm=<ice-test>, "
        r"username=<ice-test>, lifetime=(\d+)"
    )
    while time.monotonic() < deadline:
        with open(path) as log:
            output = log.read()
        entries = pattern.findall(output)
        allocated = {session for session, status, lifetime in entries
                     if status == "new" and int(lifetime) > 0}
        released = {session for session, status, lifetime in entries
                    if status == "refreshed" and int(lifetime) == 0}
        closed = set(re.findall(
            r"session (\d+): closed \(2nd stage\), user <ice-test> realm <ice-test>",
            output
        ))
        if len(allocated) >= 2 and allocated <= released and allocated <= closed:
            if os.environ.get("ICE_TEST_REQUIRE_RENEWALS") == "1":
                renewed = {session for session, status, lifetime in entries
                           if status == "refreshed" and int(lifetime) > 0}
                channel_binds = re.findall(
                    r"session (\d+): .*incoming packet CHANNEL_BIND processed, success",
                    output
                )
                if not allocated <= renewed or any(
                        channel_binds.count(session) < 2 for session in allocated):
                    raise RuntimeError("Renovações TURN de allocation/channel não confirmadas")
                print(f"TURN_RENEWED allocations={len(renewed)} channels={len(allocated)}", flush=True)
            print(f"TURN_RELEASED allocations={len(allocated)}", flush=True)
            return
        time.sleep(0.02)
    raise RuntimeError("Coturn não confirmou a liberação de todas as allocations")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--turn-log")
    args = parser.parse_args()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("0.0.0.0", 5555))
        listener.listen(2)

        wait_stun()
        print("READY", flush=True)

        while True:
            first, _ = listener.accept()
            with first:
                second, _ = listener.accept()
                with second:
                    first.settimeout(5)
                    second.settimeout(5)

                    with first.makefile("rwb") as a, second.makefile("rwb") as b:
                        local_a = read_message(a)
                        local_b = read_message(b)

                        a.write(struct.pack("!I", len(local_b)) + local_b)
                        a.flush()

                        b.write(struct.pack("!I", len(local_a)) + local_a)
                        b.flush()

                        if args.turn_log:
                            if os.environ.get("ICE_TEST_REQUIRE_RENEWALS") == "1":
                                first.settimeout(720)
                                second.settimeout(720)
                            if read_message(a) != b"CLOSED" or read_message(b) != b"CLOSED":
                                raise ValueError("Fechamento TURN não confirmado pelos peers")
                            wait_turn_release(args.turn_log)
                            a.write(b"OK")
                            a.flush()
                            b.write(b"OK")
                            b.flush()


if __name__ == "__main__":
    main()
