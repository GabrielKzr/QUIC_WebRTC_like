use anyhow::{ensure, Context, Result};
use patchbay::{Device, Firewall, IfaceConfig, Lab, Nat, Router, RouterPreset};
use std::{collections::BTreeMap, fs::File, path::Path, time::Duration};
use tokio::{io::AsyncWriteExt, process::{Child, Command}};

pub struct Program<'a> {
    pub binary: &'a Path,
    pub working_directory: &'a Path,
    pub artifacts: &'a Path,
    pub tcp_tunnel: bool,
}

pub struct Topology {
    pub devices: BTreeMap<&'static str, Device>,
    pub routers: Vec<Router>,
    processes: Vec<(&'static str, Child)>,
    _lab: Lab,
}

// Identifica a WAN pelo endereco de uplink, independentemente do nome da interface.
pub fn wan_interface(router: &Router) -> Result<String> {
    let uplink_ip = router.uplink_ip().context("router uplink IPv4")?;
    router
        .interfaces()
        .into_iter()
        .find(|iface| iface.ip() == Some(uplink_ip))
        .map(|iface| iface.name().to_owned())
        .context("router WAN interface not found")
}

// Edite aqui os nos, conexoes e enderecos. Todos os testes usam esta rede.
pub async fn build(program: &Program<'_>) -> Result<Topology> {
    let lab = Lab::new().await?;

    let isp = lab
        .add_router("isp")
        .preset(RouterPreset::Public)
        .downstream_cidr("10.0.0.0/24".parse()?)
        .build()
        .await?;

    let router_a = lab
        .add_router("router-a")
        .preset(RouterPreset::Home)
        .upstream(isp.id())
        .downstream_cidr("192.168.80.0/24".parse()?)
        .nat(Nat::Moderate)
        .firewall(Firewall::None)
        .build()
        .await?;

    let router_b = lab
        .add_router("router-b")
        .preset(RouterPreset::Home)
        .upstream(isp.id())
        .downstream_cidr("192.168.80.0/24".parse()?)
        .nat(Nat::Moderate)
        .firewall(Firewall::None)
        .build()
        .await?;

    let peer_a = lab
        .add_device("peer_a")
        .iface(
            "eth0",
            IfaceConfig::routed(router_a.id()).addr("192.168.80.20/24".parse()?),
        )
        .build()
        .await?;

    let peer_b = lab
        .add_device("peer_b")
        .iface(
            "eth0",
            IfaceConfig::routed(router_b.id()).addr("192.168.80.30/24".parse()?),
        )
        .build()
        .await?;

    for router in [&router_a, &router_b] {
        let public = router.uplink_ip().context("NAT public IPv4")?;
        let wan = wan_interface(router)?;
        // Cria o mapa somente no envio: porta publica = origem UDP.
        // A resposta volta ao endereco e porta de origem privados.
        // A filtragem APDF permanece nas regras nativas de Nat::Moderate.
        let rules = format!(
            "flush map ip nat fullcone\n\
             add set ip nat opened_ports {{ type inet_service; flags dynamic,timeout; timeout 300s; }}\n\
             add chain ip nat inbound_guard {{ type filter hook prerouting priority raw; policy accept; }}\n\
             add rule ip nat inbound_guard iif \"{wan}\" meta l4proto udp udp dport != @opened_ports drop\n\
             flush chain ip nat postrouting\n\
             add rule ip nat postrouting oif \"{wan}\" meta l4proto udp update @fullcone {{ udp sport timeout 300s : ip saddr . udp sport }}\n\
             add rule ip nat postrouting oif \"{wan}\" meta l4proto udp update @opened_ports {{ udp sport timeout 300s }}\n\
             add rule ip nat postrouting oif \"{wan}\" meta l4proto udp snat to {public}:udp sport\n"
        );
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
            "configure source-port-preserving SNAT"
        );
    }

    let peer_a_remote = router_b.uplink_ip().context("peer_b public IPv4")?;
    let peer_b_remote = router_a.uplink_ip().context("peer_a public IPv4")?;
    let mut topology = Topology {
        devices: BTreeMap::from([("peer_a", peer_a), ("peer_b", peer_b)]),
        routers: vec![isp, router_a, router_b],
        processes: Vec::new(),
        _lab: lab,
    };

    // Resumo da rede gerado a partir dos nos declarados, sem nomes fixos no runner.
    let mut summary = String::new();
    for (name, device) in &topology.devices {
        summary.push_str(&format!("{name}={}\n", device.ip().context("device IPv4")?));
    }
    for router in &topology.routers {
        summary.push_str(&format!("{}_public={}\n", router.name(), router.uplink_ip().context("router public IPv4")?));
    }
    std::fs::write(program.artifacts.join("topology.txt"), summary)?;

    let command = |role: &str, remote: String| -> Result<Command> {
        let mut cmd = Command::new("stdbuf");
        cmd.args(["-oL", "-eL"]).arg(program.binary);
        cmd.arg("-d").arg(role);
        if program.tcp_tunnel {
            cmd.arg(if role == "-s" { "1234" } else { "44444" });
        }
        cmd.arg(remote)
            .current_dir(program.working_directory)
            .kill_on_drop(true)
            .stdout(File::create(program.artifacts.join(format!("{role}.stdout.log")))?)
            .stderr(File::create(program.artifacts.join(format!("{role}.stderr.log")))?);
        Ok(cmd)
    };

    topology.processes.push((
        "peer_b",
        topology.devices["peer_b"].spawn_command(command("-s", peer_b_remote.to_string())?)?,
    ));
    tokio::time::sleep(Duration::from_millis(200)).await;
    topology.processes.push((
        "peer_a",
        topology.devices["peer_a"].spawn_command(command("-c", peer_a_remote.to_string())?)?,
    ));
    Ok(topology)
}

impl Topology {
    // Aguarda todos os programas; erro de qualquer peer interrompe a espera.
    pub async fn wait_programs(&mut self, timeout: Duration) -> Result<()> {
        let deadline = tokio::time::Instant::now() + timeout;
        loop {
            let mut pending = Vec::new();
            for (name, process) in &mut self.processes {
                match process.try_wait().with_context(|| format!("read exit status of {name}"))? {
                    Some(status) => ensure!(status.success(), "peer {name} failed: {status}"),
                    None => pending.push(*name),
                }
            }
            if pending.is_empty() {
                return Ok(());
            }
            ensure!(
                tokio::time::Instant::now() < deadline,
                "peers did not terminate within {} seconds: {}",
                timeout.as_secs(), pending.join(", ")
            );
            tokio::time::sleep(Duration::from_millis(20)).await;
        }
    }

    pub async fn stop_programs(&mut self) {
        for (_, mut child) in self.processes.drain(..) {
            let _ = child.kill().await;
        }
        // kill_on_drop cobre erros durante a preparacao ou verificacao.
    }
}
