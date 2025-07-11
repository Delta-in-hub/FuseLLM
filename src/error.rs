use std::io;
use thiserror::Error;

/// The primary error type for all operations in FuseLLM.
///
/// It encapsulates errors from underlying libraries (IO, FUSE, APIs)
/// and custom application-level errors.
#[derive(Debug, Error)]
pub enum FuseLlmError {
    #[error("I/O error: {0}")]
    Io(#[from] io::Error),

    #[error("Configuration error: {0}")]
    Config(#[from] toml::de::Error),

    #[error("ZMQ communication error: {0}")]
    Zmq(#[from] zmq::Error),

    #[error("OpenAI API error: {0}")]
    OpenAi(#[from] async_openai::error::OpenAIError),
    
    #[error("Serialization/Deserialization error: {0}")]
    Serde(#[from] serde_json::Error),

    #[error("Invalid path or entity not found")]
    NotFound,

    #[error("Permission denied")]
    AccessDenied,

    #[error("Invalid argument: {0}")]
    InvalidInput(String),

    #[error("Operation failed: {0}")]
    OperationFailed(String),
}

/// A specialized `Result` type for FuseLLM operations.
pub type Result<T> = std::result::Result<T, FuseLlmError>;
