//! # FuseLLM Library Crate
//!
//! This crate contains all the core logic for the FuseLLM filesystem.
//! The main binary in `src/main.rs` uses this library to mount the filesystem.
//! This structure makes the core logic highly testable.

// Public modules, accessible for integration testing and the main binary.
pub mod config;
pub mod error;
pub mod filesystem;
pub mod handlers;
pub mod services;
pub mod state;
pub mod vfs;
