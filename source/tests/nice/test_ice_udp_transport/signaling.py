import os
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


def main():
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


if __name__ == "__main__":
    main()
