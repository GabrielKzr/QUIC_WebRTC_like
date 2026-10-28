// Verificacoes dos programas: requisitos de devices e servico echo ficam aqui.
use crate::topology::Topology;
use anyhow::{ensure, Context, Result};
use std::{fs::File, time::Duration};
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
    process::Command,
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

    // Mantem os handles vivos para capturar durante toda a verificacao.
    let _captures = start_router_diagnostics(topology, artifacts).await?;

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
        let mut received = vec![0; payload.len()];
        tokio::try_join!(async { write.write_all(&payload).await }, async {
            read.read_exact(&mut received).await.map(|_| ())
        })?;
        ensure!(received == payload, "payload mismatch");
        write.shutdown().await?;
        anyhow::Ok(())
    })?;
    let mut probe = probe;
    let result =
        match tokio::time::timeout(Duration::from_secs(55), &mut probe)
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

// Salva regras existentes e inicia capturas; nao altera a configuracao da rede.
async fn start_router_diagnostics(
    topology: &Topology,
    artifacts: &std::path::Path,
) -> Result<Vec<tokio::process::Child>> {
    for router in &topology.routers {
        let name = router.name();
        let mut cmd = Command::new("nft");
        cmd.args(["list", "ruleset"])
            .stdout(std::process::Stdio::piped())
            .stderr(std::process::Stdio::piped());
        let output = router.spawn_command(cmd)?.wait_with_output().await?;
        ensure!(output.status.success(), "cannot inspect router rules");
        std::fs::write(artifacts.join(format!("{name}.nft")), output.stdout)?;
    }

    match start_router_captures(topology, artifacts).await {
        Ok(captures) => {
            std::fs::write(artifacts.join("capture-status.txt"), "STARTED\n")?;
            Ok(captures)
        }
        Err(error) => {
            let message = format!("Packet capture unavailable: {error:#}");
            eprintln!("Warning: {message}; continuing protocol verification");
            std::fs::write(artifacts.join("capture-status.txt"), format!("{message}\n"))?;
            Ok(Vec::new())
        }
    }
}

async fn start_router_captures(
    topology: &Topology,
    artifacts: &std::path::Path,
) -> Result<Vec<tokio::process::Child>> {
    let mut captures = Vec::new();
    for router in &topology.routers {
        let name = router.name();
        let mut cmd = Command::new("tcpdump");
        let wan = crate::topology::wan_interface(router)?;
        cmd.args(["-Z", "root", "-nn", "-l", "-i", wan.as_str(), "udp"])
            .kill_on_drop(true)
            .stdout(File::create(artifacts.join(format!("{name}.udp.log")))?)
            .stderr(File::create(artifacts.join(format!("{name}.tcpdump.log")))?);
        captures.push(router.spawn_command(cmd)?);
    }
    tokio::time::sleep(Duration::from_millis(200)).await;
    for child in &mut captures {
        ensure!(child.try_wait()?.is_none(), "tcpdump exited before probe");
    }
    Ok(captures)
}
