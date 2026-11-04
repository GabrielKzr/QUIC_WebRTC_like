mod topology;
mod patchbay;

use anyhow::{ensure, Context, Result};
use std::path::PathBuf;

fn main() -> Result<()> {
    ::patchbay::init_userns().context(
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
        args.len() >= 4,
        "usage: runner <peer-binary> <tcp|udp|tcp-long> <source-dir> <artifacts-dir> [isp-script] [--nat-rules <file>]"
    );
    let binary = std::fs::canonicalize(&args[0])?;
    let test_kind = args[1].to_str().context("invalid test kind")?;
    ensure!(
        matches!(test_kind, "tcp" | "udp" | "tcp-long"),
        "unknown test kind"
    );
    let cwd = std::fs::canonicalize(&args[2])?;
    let artifacts = PathBuf::from(&args[3]);
    let mut isp_script = None;
    let mut nat_extra_rules = None;
    let mut optional = args[4..].iter();
    while let Some(arg) = optional.next() {
        if arg == "--nat-rules" {
            ensure!(nat_extra_rules.is_none(), "duplicate --nat-rules");
            let path = optional.next().context("--nat-rules requires a file")?;
            nat_extra_rules = Some(std::fs::read_to_string(path)?);
        } else {
            ensure!(isp_script.is_none(), "unexpected runner argument");
            isp_script = Some(std::fs::canonicalize(arg)?);
        }
    }
    std::fs::create_dir_all(&artifacts)?;
    std::fs::write(artifacts.join("result.txt"), "RUNNING\n")?;
    let result = exercise(binary, cwd, &artifacts, test_kind, isp_script, nat_extra_rules).await;
    std::fs::write(
        artifacts.join("result.txt"),
        match &result {
            Ok(()) => "PASS: verification and process exits succeeded\n".to_owned(),
            Err(e) => format!("FAIL: {e:#}\n"),
        },
    )?;
    result
}

async fn exercise(
    binary: PathBuf,
    cwd: PathBuf,
    artifacts: &std::path::Path,
    test_kind: &str,
    isp_script: Option<PathBuf>,
    nat_extra_rules: Option<String>,
) -> Result<()> {
    let mut topology = topology::build(&topology::Program {
        binary: &binary,
        working_directory: &cwd,
        artifacts,
        tcp_tunnel: test_kind != "udp",
        isp_script: isp_script.as_deref(),
        nat_extra_rules: nat_extra_rules.as_deref(),
    })
    .await?;
    let result = patchbay::verify(&mut topology, &binary, artifacts, test_kind).await;
    topology.stop_programs().await;
    result
}
