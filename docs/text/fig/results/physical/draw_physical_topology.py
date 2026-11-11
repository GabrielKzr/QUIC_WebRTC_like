"""Generate a standalone SVG with embedded icons and an opaque white background."""

from html import escape
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
SVG_NS = "http://www.w3.org/2000/svg"
parts = [
    '<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="900" '
    'viewBox="0 0 1200 900" role="img" aria-labelledby="title description">',
    '<title id="title">Topologia física dos testes QUIC sobre ICE</title>',
    '<desc id="description">Roteador base 10.0.0.1 fornece a rede 10.0.0.0/24 '
    'ao servidor 10.0.0.108 e às WANs dos NATs A e B. Cada NAT atende uma LAN '
    '192.168.80.0/24 separada com um peer/gateway.</desc>',
    '<rect x="0" y="0" width="1200" height="900" fill="#ffffff"/>',
    '<g font-family="DejaVu Sans, Arial, sans-serif" fill="#111111">',
]


def text(x, y, value, size=20, weight="normal"):
    parts.append(
        f'<text x="{x}" y="{y}" text-anchor="middle" '
        f'font-size="{size}" font-weight="{weight}">{escape(value)}</text>'
    )


def line(x1, y1, x2, y2):
    parts.append(
        f'<path d="M{x1} {y1} L{x2} {y2}" fill="none" '
        'stroke="#333333" stroke-width="2.5" stroke-linecap="round"/>'
    )


def icon(name, x, y, size=90):
    source = ET.parse(ROOT / "icons" / f"{name}.svg").getroot()
    paths = list(source.iter(f"{{{SVG_NS}}}path"))
    assert paths, name
    parts.append(f'<g transform="translate({x - size / 2} {y}) scale({size / 24})">')
    for path in paths:
        parts.append(f'<path fill="#111111" d="{escape(path.attrib["d"], quote=True)}"/>')
    parts.append('</g>')


# Shared upstream network. The server is a host attached to this network.
icon("router-wireless", 430, 35)
text(430, 151, "Roteador base", weight="bold")
text(430, 184, "eth0", size=18)
text(430, 208, "10.0.0.1/24")
line(430, 220, 430, 285)

icon("server", 960, 35)
text(960, 151, "Servidor STUN/TURN", weight="bold")
text(960, 177, "e sinalização", size=18)
text(960, 208, "eth0", size=18)
text(960, 232, "10.0.0.108")
line(960, 244, 960, 285)

line(270, 285, 960, 285)
text(670, 267, "Rede 10.0.0.0/24", size=20)
for x in [270, 430, 740, 960]:
    parts.append(f'<circle cx="{x}" cy="285" r="4" fill="#333333"/>')

for letter, x, public_ip, peer_ip in [
    ("A", 270, "10.0.0.117", "192.168.80.100"),
    ("B", 740, "10.0.0.40", "192.168.80.101"),
]:
    line(x, 285, x, 326)
    text(x, 350, "eth0 — WAN", size=18)
    text(x, 376, public_ip)
    icon("router-wireless", x, 393)
    text(x, 509, f"Roteador NAT {letter}", weight="bold")
    text(x, 543, "eth1 — LAN", size=18)
    text(x, 569, "192.168.80.1/24")
    line(x, 583, x, 650)
    text(x + 122, 606, f"LAN {letter}", size=16)
    text(x + 122, 628, "192.168.80.0/24", size=16)
    text(x, 674, "eth0", size=18)
    text(x, 700, peer_ip)
    icon("router-network", x, 716)
    text(x, 832, f"Peer/Gateway {letter}", weight="bold")

text(600, 880, "Interfaces com nomes ilustrativos; servidor e Peer/Gateway A no mesmo PC físico.", size=15)
parts.extend(['</g>', '</svg>'])
(ROOT / "physical_topology.svg").write_text("\n".join(parts) + "\n", encoding="utf-8")
