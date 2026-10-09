use anyhow::{ensure, Context, Result};
use patchbay::{Firewall, Lab, Nat, RouterPreset};
use std::{fs::File, path::PathBuf, time::Duration};
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
    process::Command,
};

fn main() -> Result<()> {
    patchbay::init_userns().context(
        "Patchbay requires permitted Linux user namespaces; check AppArmor/container policy",
    )?;
    tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()?
        .block_on(run())
}

async fn run() -> Result<()> {
    let args: Vec<_> = std::env::args_os().skip(1).collect();
    ensure!(
        args.len() == 4,
        "usage: runner <peer-binary> <public|nat> <source-dir> <artifacts-dir>"
    );
    let binary = std::fs::canonicalize(&args[0])?;
    let scenario = args[1].to_str().context("invalid scenario")?;
    ensure!(
        matches!(
            scenario,
            "public" | "nat" | "smoke-nat" | "gns3" | "udp-gns3" | "udp-nat"
        ),
        "unknown scenario"
    );
    let cwd = std::fs::canonicalize(&args[2])?;
    let artifacts = PathBuf::from(&args[3]);
    std::fs::create_dir_all(&artifacts)?;
    std::fs::write(artifacts.join("result.txt"), "RUNNING\n")?;
    let result = exercise(binary, cwd, &artifacts, scenario).await;
    std::fs::write(
        artifacts.join("result.txt"),
        match &result {
            Ok(()) => "PASS: payload verified\n".to_owned(),
            Err(e) => format!("FAIL: {e:#}\n"),
        },
    )?;
    result
}

async fn exercise(
    binary: PathBuf,
    cwd: PathBuf,
    artifacts: &std::path::Path,
    scenario: &str,
) -> Result<()> {
    let lab = Lab::new().await?;
    let direct = matches!(scenario, "public" | "gns3" | "udp-gns3");
    let gns3 = matches!(scenario, "gns3" | "udp-gns3");
    let preset = if !direct {
        RouterPreset::Home
    } else {
        RouterPreset::PublicV4
    };
    let a = lab
        .add_router("nat-a")
        .preset(preset)
        .nat(if direct { Nat::None } else { Nat::Open })
        .firewall(Firewall::None)
        .build()
        .await?;
    let b = lab
        .add_router("nat-b")
        .preset(preset)
        .nat(if direct { Nat::None } else { Nat::Open })
        .firewall(Firewall::None)
        .build()
        .await?;
    let client = lab
        .add_device("client")
        .iface("eth0", a.id())
        .build()
        .await?;
    let server = lab
        .add_device("server")
        .iface("eth0", if gns3 { a.id() } else { b.id() })
        .build()
        .await?;
    if gns3 {
        for (device, address) in [(&client, "10.0.0.20/24"), (&server, "10.0.0.30/24")] {
            let mut cmd = Command::new("ip");
            cmd.args(["addr", "add", address, "dev", "eth0"]);
            ensure!(
                device.spawn_command(cmd)?.wait().await?.success(),
                "configure peer IP"
            );
        }
    }
    let client_remote = if gns3 {
        Some("10.0.0.30".parse()?)
    } else if !direct {
        b.uplink_ip()
    } else {
        server.ip()
    }
    .context("server IPv4")?;
    let server_remote = if gns3 {
        Some("10.0.0.20".parse()?)
    } else if !direct {
        a.uplink_ip()
    } else {
        client.ip()
    }
    .context("client IPv4")?;
    std::fs::write(artifacts.join("scenario.txt"), format!("scenario={scenario}\nclient_remote={client_remote}\nserver_remote={server_remote}\nbinary={}\n", binary.display()))?;

    if !direct {
        for (router, device) in [(&a, &client), (&b, &server)] {
            let public = router.uplink_ip().context("NAT public IPv4")?;
            let private = device.ip().context("NAT private IPv4")?;
            let rules = format!("add element ip nat fullcone {{ 2222 timeout 300s : {private} . 2222 }}\nflush chain ip nat postrouting\nadd rule ip nat postrouting oif \"ix\" meta l4proto udp update @fullcone {{ udp sport timeout 300s : ip saddr . udp sport }}\nadd rule ip nat postrouting oif \"ix\" udp sport 2222 snat to {public}:2222\n");
            let mut cmd = Command::new("nft");
            cmd.args(["-f", "-"]).stdin(std::process::Stdio::piped());
            let mut child = router.spawn_command(cmd)?;
            child
                .stdin
                .take()
                .context("nft stdin")?
                .write_all(rules.as_bytes())
                .await?;
            ensure!(
                child.wait().await?.success(),
                "configure port-preserving SNAT"
            );
        }
    }

    for (name, router) in [("router-a", &a), ("router-b", &b)] {
        let mut cmd = Command::new("nft");
        cmd.args(["list", "ruleset"])
            .stdout(std::process::Stdio::piped())
            .stderr(std::process::Stdio::piped());
        let output = router.spawn_command(cmd)?.wait_with_output().await?;
        ensure!(output.status.success(), "cannot inspect router rules");
        std::fs::write(artifacts.join(format!("{name}.nft")), output.stdout)?;
    }

    let mut captures = Vec::new();
    if !direct {
        for (name, router) in [("router-a", &a), ("router-b", &b)] {
            let mut cmd = Command::new("tcpdump");
            cmd.args(["-Z", "root", "-nn", "-l", "-i", "ix", "udp", "port", "2222"])
                .kill_on_drop(true)
                .stdout(File::create(artifacts.join(format!("{name}.udp.log")))?)
                .stderr(File::create(artifacts.join(format!("{name}.tcpdump.log")))?);
            captures.push(router.spawn_command(cmd)?);
        }
        tokio::time::sleep(Duration::from_millis(200)).await;
        for child in &mut captures {
            ensure!(child.try_wait()?.is_none(), "tcpdump exited before probe");
        }
    }

    if scenario == "smoke-nat" {
        let udp_probe = |remote: std::net::Ipv4Addr| async move {
            let socket = tokio::net::UdpSocket::bind("0.0.0.0:2222").await?;
            let mut buf = [0; 64];
            for _ in 0..50 {
                socket
                    .send_to(b"patchbay-nat-probe", (remote, 2222))
                    .await?;
                if let Ok(Ok((n, _))) =
                    tokio::time::timeout(Duration::from_millis(100), socket.recv_from(&mut buf))
                        .await
                {
                    ensure!(&buf[..n] == b"patchbay-nat-probe", "UDP payload mismatch");
                    // Continue sending briefly so the other peer also observes a reply.
                    for _ in 0..5 {
                        socket
                            .send_to(b"patchbay-nat-probe", (remote, 2222))
                            .await?;
                        tokio::time::sleep(Duration::from_millis(20)).await;
                    }
                    return anyhow::Ok(());
                }
            }
            anyhow::bail!("UDP NAT probe timed out")
        };
        let a = client.spawn(move |_| udp_probe(client_remote))?;
        let b = server.spawn(move |_| udp_probe(server_remote))?;
        a.await??;
        b.await??;
        return Ok(());
    }

    if matches!(scenario, "udp-gns3" | "udp-nat") {
        let command = |role: &str, remote: &str| -> Result<Command> {
            let mut cmd = Command::new("stdbuf");
            cmd.args(["-oL", "-eL"])
                .arg(&binary)
                .args(["-d", role, remote])
                .current_dir(&cwd)
                .kill_on_drop(true)
                .stdout(File::create(artifacts.join(format!("{role}.stdout.log")))?)
                .stderr(File::create(artifacts.join(format!("{role}.stderr.log")))?);
            Ok(cmd)
        };
        let mut s = server.spawn_command(command("-s", &server_remote.to_string())?)?;
        tokio::time::sleep(Duration::from_millis(200)).await;
        let mut c = client.spawn_command(command("-c", &client_remote.to_string())?)?;
        let mut passed = false;
        for _ in 0..100 {
            let sl = std::fs::read_to_string(artifacts.join("-s.stdout.log"))?;
            let cl = std::fs::read_to_string(artifacts.join("-c.stdout.log"))?;
            if sl.contains("Hello from client") && cl.contains("Hello from server") {
                passed = true;
                break;
            }
            tokio::time::sleep(Duration::from_millis(100)).await;
        }
        let _ = c.kill().await;
        let _ = s.kill().await;
        ensure!(
            passed,
            "UDP example did not exchange messages in both directions"
        );
        return Ok(());
    }

    let (ready_tx, ready_rx) = tokio::sync::oneshot::channel();
    let echo = server.spawn(move |_| async move {
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
    })?;
    tokio::time::timeout(Duration::from_secs(5), ready_rx).await??;
    let peer_command = |role: &str, remote: String| -> Result<Command> {
        let mut cmd = Command::new("stdbuf");
        cmd.arg("-oL").arg("-eL").arg(&binary);
        cmd.current_dir(&cwd)
            .args([
                "-d",
                role,
                if role == "-s" { "1234" } else { "44444" },
                &remote,
            ])
            .kill_on_drop(true)
            .stdout(File::create(artifacts.join(format!("{role}.stdout.log")))?)
            .stderr(File::create(artifacts.join(format!("{role}.stderr.log")))?);
        Ok(cmd)
    };
    let mut server_peer = server.spawn_command(peer_command("-s", server_remote.to_string())?)?;
    tokio::time::sleep(Duration::from_millis(200)).await;
    let mut client_peer = client.spawn_command(peer_command("-c", client_remote.to_string())?)?;
    let probe = client.spawn(move |_| async move {
        let mut stream = loop {
            match TcpStream::connect("127.0.0.1:44444").await {
                Ok(s) => break s,
                Err(_) => tokio::time::sleep(Duration::from_millis(50)).await,
            }
        };
        // Includes zeros and multiple protocol buffers; catches string truncation.
        let payload: Vec<u8> = if gns3 {
            b"hello chownat\n".to_vec()
        } else {
            (0..16384).map(|n| (n % 251) as u8).collect()
        };
        // Read concurrently so neither side depends on socket buffer capacity.
        let (mut read, mut write) = stream.split();
        let mut received = vec![0; payload.len()];
        tokio::try_join!(async { write.write_all(&payload).await }, async {
            read.read_exact(&mut received).await.map(|_| ())
        })?;
        ensure!(received == payload, "payload mismatch");
        anyhow::Ok(())
    })?;
    let mut probe = probe;
    let result =
        match tokio::time::timeout(Duration::from_secs(if gns3 { 10 } else { 55 }), &mut probe)
            .await
        {
            Ok(joined) => joined.context("probe task failed").and_then(|r| r),
            Err(_) => {
                probe.abort();
                Err(anyhow::anyhow!("transfer exceeded scenario deadline"))
            }
        };
    // Explicitly reap external processes before releasing the lab.
    let _ = client_peer.kill().await;
    let _ = server_peer.kill().await;
    echo.abort();
    let _ = echo.await;
    result
}
