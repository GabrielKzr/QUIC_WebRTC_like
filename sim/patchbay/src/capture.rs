use crate::topology::Topology;
use anyhow::{ensure, Context, Result};
use nix::{sys::signal::{kill, Signal}, unistd::Pid};
use std::{fs::{File, OpenOptions}, io::Write, path::{Path, PathBuf}, time::Duration};
use tokio::process::{Child, Command};

struct Capture {
    child: Child,
    path: PathBuf,
}

pub struct Captures {
    children: Vec<Capture>,
    status: PathBuf,
    finished: bool,
    failed: bool,
}

impl Captures {
    fn record(&self, message: &str) -> Result<()> {
        writeln!(OpenOptions::new().append(true).open(&self.status)?, "{message}")?;
        Ok(())
    }

    async fn start_node<F>(&mut self, name: &str, spawn: F, artifacts: &Path) -> Result<()>
    where F: Fn(Command) -> Result<Child> {
        let directory = artifacts.join("pcap").join(name);
        std::fs::create_dir_all(&directory)?;
        let mut command = Command::new("ip");
        command.args(["-o", "link", "show"])
            .stdout(std::process::Stdio::piped()).stderr(std::process::Stdio::piped());
        let output = spawn(command)?.wait_with_output().await?;
        ensure!(output.status.success(), "list interfaces of {name}");
        let links = String::from_utf8(output.stdout)?;
        std::fs::write(directory.join("interfaces.txt"), &links)?;
        let mut command = Command::new("ip");
        command.args(["-o", "addr", "show"])
            .stdout(std::process::Stdio::piped()).stderr(std::process::Stdio::piped());
        let output = spawn(command)?.wait_with_output().await?;
        ensure!(output.status.success(), "list addresses of {name}");
        std::fs::write(directory.join("addresses.txt"), output.stdout)?;

        let mut count = 0;
        for line in links.lines() {
            let interface = line.split_once(':').and_then(|(_, tail)| tail.split_once(':'))
                .map(|(interface, _)| interface.trim().split('@').next().unwrap_or(""))
                .context("parse interface from ip link")?;
            ensure!(!interface.is_empty() && !interface.contains('/'), "invalid interface: {interface}");
            let path = directory.join(format!("{interface}.pcap"));
            let log = directory.join(format!("{interface}.tcpdump.log"));
            let mut command = Command::new("tcpdump");
            command.args(["-n", "-p", "-U", "--immediate-mode", "-s", "0", "-i", interface, "-w", "-"])
                .env("LC_ALL", "C")
                .kill_on_drop(true)
                .stdin(std::process::Stdio::null())
                .stdout(File::create(&path)?)
                .stderr(File::create(&log)?);
            let child = spawn(command).with_context(|| format!("start capture {name}/{interface}"))?;
            self.children.push(Capture { child, path: path.clone() });
            let capture = self.children.last_mut().unwrap();
            let deadline = tokio::time::Instant::now() + Duration::from_secs(3);
            loop {
                ensure!(capture.child.try_wait()?.is_none(), "capture {name}/{interface} exited: {}", std::fs::read_to_string(&log)?);
                // The pcap header is written after the capture device is activated.
                if std::fs::metadata(&path)?.len() >= 24 {
                    break;
                }
                ensure!(tokio::time::Instant::now() < deadline, "capture {name}/{interface} not ready; see {}", log.display());
                tokio::time::sleep(Duration::from_millis(10)).await;
            }
            self.record(&format!("STARTED {name}/{interface} {}", path.display()))?;
            count += 1;
        }
        ensure!(count > 0, "no interfaces found for {name}");
        Ok(())
    }

    pub fn check(&mut self) -> Result<()> {
        for capture in &mut self.children {
            if let Some(status) = capture.child.try_wait()? {
                self.failed = true;
                anyhow::bail!("capture {} exited early: {status}; see {}", capture.path.display(), capture.path.with_extension("tcpdump.log").display());
            }
        }
        Ok(())
    }

    pub async fn stop(&mut self) -> Result<()> {
        let mut errors = Vec::new();
        // Let activated capturers drain the final close/data packets before SIGINT.
        if !self.children.is_empty() {
            tokio::time::sleep(Duration::from_millis(100)).await;
        }
        for capture in &mut self.children {
            match capture.child.try_wait() {
                Ok(Some(status)) => errors.push(format!("{} exited early: {status}", capture.path.display())),
                Ok(None) => {
                    if let Some(pid) = capture.child.id() {
                        if let Err(error) = kill(Pid::from_raw(pid as i32), Signal::SIGINT) {
                            errors.push(format!("signal {}: {error}", capture.path.display()));
                        }
                    }
                }
                Err(error) => errors.push(format!("inspect {}: {error}", capture.path.display())),
            }
        }
        for mut capture in self.children.drain(..) {
            match tokio::time::timeout(Duration::from_secs(2), capture.child.wait()).await {
                Ok(Ok(status)) if status.success() => {}
                Ok(result) => errors.push(format!("stop {}: {result:?}", capture.path.display())),
                Err(_) => {
                    let _ = capture.child.kill().await;
                    let _ = capture.child.wait().await;
                    errors.push(format!("forced stop of {}", capture.path.display()));
                }
            }
        }
        self.finished = true;
        self.failed |= !errors.is_empty();
        for error in &errors {
            self.record(&format!("ERROR {error}"))?;
        }
        self.record(if self.failed { "FAILED" } else { "COMPLETE" })?;
        ensure!(!self.failed, "pcap capture failed; see {}", self.status.display());
        Ok(())
    }
}

impl Drop for Captures {
    fn drop(&mut self) {
        if !self.finished {
            // Last resort on cancellation/panic; normal paths await stop().
            for capture in &mut self.children {
                if let Some(pid) = capture.child.id() {
                    let _ = kill(Pid::from_raw(pid as i32), Signal::SIGINT);
                }
            }
            let _ = self.record("ABORTED: capture cleanup interrupted");
        }
    }
}

pub async fn start(topology: &Topology, artifacts: &Path) -> Result<Option<Captures>> {
    // Rule inspection remains mandatory, independently of packet capture.
    for router in &topology.routers {
        let mut command = Command::new("nft");
        command.args(["list", "ruleset"])
            .stdout(std::process::Stdio::piped()).stderr(std::process::Stdio::piped());
        let output = router.spawn_command(command)?.wait_with_output().await?;
        ensure!(output.status.success(), "inspect rules of {}", router.name());
        std::fs::write(artifacts.join(format!("{}.nft", router.name())), output.stdout)?;
    }
    let status = artifacts.join("capture-status.txt");
    if std::env::var_os("PATCHBAY_PCAP").is_none() {
        std::fs::write(status, "DISABLED: PATCHBAY_PCAP is absent\n")?;
        return Ok(None);
    }
    // Reusing an artifacts directory must not mix interface files from old runs.
    let directory = artifacts.join("pcap");
    if directory.exists() {
        std::fs::remove_dir_all(&directory)?;
    }
    std::fs::write(&status, "STARTING\n")?;
    let mut captures = Captures { children: Vec::new(), status, finished: false, failed: false };
    let result = async {
        for router in &topology.routers {
            captures.start_node(router.name(), |cmd| router.spawn_command(cmd), artifacts).await?;
        }
        for (name, device) in &topology.devices {
            captures.start_node(name, |cmd| device.spawn_command(cmd), artifacts).await?;
        }
        captures.check()?;
        anyhow::Ok(())
    }.await;
    if let Err(error) = result {
        captures.failed = true;
        let _ = captures.record(&format!("ERROR {error:#}"));
        let _ = captures.stop().await;
        return Err(error);
    }
    Ok(Some(captures))
}
