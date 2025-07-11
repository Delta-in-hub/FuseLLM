use clap::Parser;
use fuser::mount2;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};

use fusellm::config::GlobalConfig;
use fusellm::fs::FuseLlm;
use fusellm::state::FilesystemState;

#[derive(Parser, Debug)] // 让 `Args` 自动实现 `clap::Parser` 和 `Debug`
#[command(
    author,         // 从 Cargo.toml 读取作者信息
    version,        // 从 Cargo.toml 读取版本号
    about,          // 从 Cargo.toml 读取简短描述
)]
struct Args {
    /// Mount point for the filesystem (required)
    mountpoint: PathBuf, // 必需的位置参数

    /// Path to configuration file (default: ./settings.toml)
    #[arg(
        short,            // 允许 `-c` 短参数
        long,             // 允许 `--config` 长参数
        default_value = ".settings.toml",  // 默认值
    )]
    config: PathBuf,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 1. Setup logging
    env_logger::init();

    // 2. Parse command-line arguments
    let args = Args::parse();
    let mountpoint: PathBuf = args.mountpoint;
    let config_path: PathBuf = args.config;

    // 3. Load configuration
    let config = GlobalConfig::load(&config_path)?;
    config.validate()?;

    // 4. Initialize state
    let state = FilesystemState::new(config);
    let state = Arc::new(Mutex::new(state));

    // 5. Create the FUSE filesystem instance
    let filesystem = FuseLlm {
        state: state.clone(),
    };

    // 6. Mount the filesystem
    println!("Mounting FuseLLM at {}", mountpoint.display());
    mount2(filesystem, &mountpoint, &[])?;

    Ok(())
}
