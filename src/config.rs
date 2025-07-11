use crate::error::Result;
use serde::Deserialize;
use std::collections::HashMap;
use std::path::Path;

/// Represents the top-level structure of the `config.toml` file.
#[derive(Debug, Deserialize, Clone)]
pub struct Config {
    pub api_key: Option<String>,
    pub default_model: DefaultModelConfig,
    #[serde(default)]
    pub models: HashMap<String, ModelConfig>,
    pub semantic_search: SemanticSearchConfig,
}

/// Configuration for the default model behavior.
#[derive(Debug, Deserialize, Clone)]
pub struct DefaultModelConfig {
    pub name: String,
    pub temperature: Option<f32>,
    pub system_prompt: String,
}

/// Per-model configuration overrides.
#[derive(Debug, Deserialize, Clone)]
pub struct ModelConfig {
    pub temperature: Option<f32>,
    pub system_prompt: Option<String>,
}

/// Configuration for the semantic search service.
#[derive(Debug, Deserialize, Clone)]
pub struct SemanticSearchConfig {
    pub zmq_address: String,
    pub embedding_model: String,
}

impl Config {
    /// Loads configuration from a specified TOML file path.
    /// It also merges settings from environment variables.
    pub fn load(_path: &Path) -> Result<Self> {
        unimplemented!()
    }

    /// Validates the loaded configuration.
    pub fn validate(&self) -> std::result::Result<(), String> {
        unimplemented!()
    }
}
