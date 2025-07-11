use anyhow::{Result, anyhow};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

/// 代表 FuseLLM 的全局设置。
/// derive(Default) 使得我们可以轻松创建一个包含所有硬编码默认值的实例。
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct GlobalConfig {
    pub default_model: Option<String>,

    // #[serde(default)] 告诉 serde：如果 TOML 文件中缺少 `default_config` 这个 table，
    // 就调用 ModelConfig::default() 来创建它。
    #[serde(default)]
    pub default_config: ModelConfig,

    pub base_url: Option<String>,
    pub api_key: String,

    // 同样，如果缺少 `semantic_search` table，就使用其默认值。
    #[serde(default)]
    pub semantic_search: SemanticSearchConfig,
}

impl GlobalConfig {
    /// 从指定的 TOML 文件路径加载配置。
    /// 如果文件不存在，则返回一个包含所有硬编码默认值的配置。
    pub fn load(path: &Path) -> Result<Self> {
        if !path.exists() {
            // 文件不存在，返回完全默认的配置
            println!(
                "Config file not found at {:?}, using default settings.",
                path
            );
            return Ok(GlobalConfig::default());
        }

        let content = fs::read_to_string(path)?;
        // toml::from_str 会利用 serde 的能力，自动处理 #[serde(default)]
        let config: GlobalConfig = toml::from_str(&content)?;

        // 加载后进行验证
        config.validate()?;

        Ok(config)
    }

    /// 验证整个配置的有效性。
    pub fn validate(&self) -> Result<()> {
        // 调用子配置的验证方法
        self.default_config.validate()?;
        // 如果 SemanticSearchConfig 也有验证逻辑，也在这里调用
        // self.semantic_search.validate()?;
        Ok(())
    }
}

/// 代表模型特定设置。
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ModelConfig {
    pub temperature: Option<f32>,
    pub system_prompt: Option<String>,
}

/// 为 ModelConfig 实现 Default trait，定义硬编码的默认值。
impl Default for ModelConfig {
    fn default() -> Self {
        Self {
            temperature: Some(1.0),
            system_prompt: Some(
                "You are a helpful assistant. Everything is a file. Even the LLM.".to_string(),
            ),
        }
    }
}

impl ModelConfig {
    /// 验证模型配置。
    pub fn validate(&self) -> Result<()> {
        if let Some(temp) = self.temperature {
            if !(0.0..=2.0).contains(&temp) {
                return Err(anyhow!(
                    "Temperature must be between 0.0 and 2.0, but got {}",
                    temp
                ));
            }
        }
        Ok(())
    }

    /// 将另一个配置合并到此配置中，`other` 的配置优先。
    pub fn merge(&mut self, other: &ModelConfig) {
        if let Some(temp) = other.temperature {
            self.temperature = Some(temp);
        }
        if let Some(prompt) = &other.system_prompt {
            self.system_prompt = Some(prompt.clone());
        }
    }
}

/// 代表语义搜索服务的设置。
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SemanticSearchConfig {
    pub service_url: Option<String>,
    pub embedding_model: Option<String>,
}

/// 为 SemanticSearchConfig 实现 Default trait，定义硬编码的默认值。
impl Default for SemanticSearchConfig {
    fn default() -> Self {
        Self {
            service_url: Some("ipc:///tmp/fusellm-semantic.ipc".to_string()),
            embedding_model: None,
        }
    }
}
