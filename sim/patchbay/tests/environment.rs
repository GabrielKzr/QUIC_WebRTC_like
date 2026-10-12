// Verificacao preservada, fora do runner e do CTest por enquanto.
use anyhow::{ensure, Context, Result};
use crate::topology::Topology;
use std::time::Duration;

pub async fn verify(topology: &Topology) -> Result<()> {
    let client = topology.devices.get("client").context("device client")?;
    let server = topology.devices.get("server").context("device server")?;
    let client_remote = topology.routers.iter().find(|r| r.name() == "router-b").context("router-b")?.uplink_ip().context("server public IPv4")?;
    let server_remote = topology.routers.iter().find(|r| r.name() == "router-a").context("router-a")?.uplink_ip().context("client public IPv4")?;
    let udp_probe = |remote: std::net::Ipv4Addr, local_port: u16, remote_port: u16| async move {
        // Portas distintas nos peers provam que SNAT preserva a origem.
        let socket = tokio::net::UdpSocket::bind(("0.0.0.0", local_port)).await?;
        let mut buf = [0; 64];
        for _ in 0..50 {
            socket
                .send_to(b"patchbay-nat-probe", (remote, remote_port))
                .await?;
            if let Ok(Ok((n, source))) =
                tokio::time::timeout(Duration::from_millis(100), socket.recv_from(&mut buf))
                    .await
            {
                ensure!(
                    source.ip() == remote && source.port() == remote_port,
                    "unexpected public endpoint: {source}"
                );
                ensure!(&buf[..n] == b"patchbay-nat-probe", "UDP payload mismatch");
                // Continue sending briefly so the other peer also observes a reply.
                for _ in 0..5 {
                    socket
                        .send_to(b"patchbay-nat-probe", (remote, remote_port))
                        .await?;
                    tokio::time::sleep(Duration::from_millis(20)).await;
                }
                return anyhow::Ok(());
            }
        }
        anyhow::bail!("UDP NAT probe timed out")
    };
    for (client_port, server_port) in [(2222, 1234), (12222, 11234)] {
        let a = client.spawn(move |_| udp_probe(client_remote, client_port, server_port))?;
        let b = server.spawn(move |_| udp_probe(server_remote, server_port, client_port))?;
        a.await??;
        b.await??;
    }
    return Ok(());
}
