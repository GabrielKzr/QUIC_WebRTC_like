// Verificacoes dos programas: requisitos de devices e servico echo ficam aqui.
use crate::topology::Topology;
use anyhow::{ensure, Context, Result};
use std::time::Duration;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
};

pub async fn verify(
    topology: &mut Topology,
    binary: &std::path::Path,
    artifacts: &std::path::Path,
    test_kind: &str,
) -> Result<()> {
    let peer_a = topology.devices.get("peer_a").context("test requires device peer_a")?;
    let peer_b = topology.devices.get("peer_b").context("test requires device peer_b")?;
    std::fs::write(
        artifacts.join("test.txt"),
        format!("test={test_kind}\nbinary={}\n", binary.display()),
    )?;

    if test_kind == "udp" {
        // O programa de teste valida UDP e deve terminar com codigo zero.
        return topology.wait_programs(Duration::from_secs(10)).await;
    }

    // Servico auxiliar deste teste, independente da criacao da topologia.
    let (ready_tx, ready_rx) = tokio::sync::oneshot::channel();
    let mut echo = Echo(peer_b.spawn(move |_| async move {
        let listener = TcpListener::bind("127.0.0.1:1234").await?;
        let _ = ready_tx.send(());
        let (mut stream, _) = listener.accept().await?;
        let mut buf = [0; 8192];
        loop {
            let n = stream.read(&mut buf).await?;
            if n == 0 {
                break;
            }
            stream.write_all(&buf[..n]).await?;
        }
        anyhow::Ok(())
    })?);
    tokio::time::timeout(Duration::from_secs(5), ready_rx).await??;

    let long_session = test_kind == "tcp-long";
    let probe = peer_a.spawn(move |_| async move {
        let mut stream = loop {
            match TcpStream::connect("127.0.0.1:44444").await {
                Ok(s) => break s,
                Err(_) => tokio::time::sleep(Duration::from_millis(50)).await,
            }
        };
        // Includes zeros and multiple protocol buffers; catches string truncation.
        let payload: Vec<u8> = (0..16384).map(|n| (n % 251) as u8).collect();
        // Read concurrently so neither side depends on socket buffer capacity.
        let (mut read, mut write) = stream.split();
        let rounds = if long_session { 67 } else { 1 };
        for round in 0..rounds {
            if round > 0 {
                tokio::time::sleep(Duration::from_secs(10)).await;
            }
            let mut received = vec![0; payload.len()];
            tokio::try_join!(async { write.write_all(&payload).await }, async {
                read.read_exact(&mut received).await.map(|_| ())
            })?;
            ensure!(received == payload, "payload mismatch in round {}", round + 1);
        }
        write.shutdown().await?;
        anyhow::Ok(())
    })?;
    let mut probe = probe;
    let result =
        match tokio::time::timeout(Duration::from_secs(if long_session { 710 } else { 55 }), &mut probe)
            .await
        {
            Ok(joined) => joined.context("probe task failed").and_then(|r| r),
            Err(_) => {
                probe.abort();
                Err(anyhow::anyhow!("transfer exceeded test_kind deadline"))
            }
        };
    result?;
    // O probe fechou o TCP; peers e echo devem encerrar normalmente.
    tokio::try_join!(
        topology.wait_programs(Duration::from_secs(10)),
        async {
            tokio::time::timeout(Duration::from_secs(10), &mut echo.0)
                .await.context("TCP echo did not terminate within 10 seconds")?
                .context("TCP echo task failed")??;
            anyhow::Ok(())
        }
    )?;
    Ok(())
}

struct Echo(tokio::task::JoinHandle<Result<()>>);

impl Drop for Echo {
    fn drop(&mut self) {
        self.0.abort();
    }
}
