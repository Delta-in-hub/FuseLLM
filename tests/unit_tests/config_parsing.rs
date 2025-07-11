//! # Unit Tests for Configuration Parsing

use fusellm::config::{GlobalConfig, ModelConfig};

#[test]
fn test_valid_config_deserialization() {
    let toml_str = r#"
[model]
default_model = "gpt-4"
temperature = 1.2

[api_keys]
openai = "sk-12345"
"#;
    let config: GlobalConfig = toml::from_str(toml_str).unwrap();
    assert_eq!(config.model.default_model, Some("gpt-4".to_string()));
    assert_eq!(config.model.temperature, Some(1.2));
    assert_eq!(config.api_keys.get("openai"), Some(&"sk-12345".to_string()));
}

#[test]
fn test_config_validation() {
    let mut config = GlobalConfig::default();
    config.model.temperature = Some(2.5);
    assert!(config.validate().is_err());

    config.model.temperature = Some(1.5);
    assert!(config.validate().is_ok());
}

#[test]
fn test_model_config_merge() {
    let mut base_config = ModelConfig {
        default_model: Some("gpt-3.5".to_string()),
        temperature: Some(0.8),
        system_prompt: Some("You are a helpful assistant.".to_string()),
    };

    let override_config = ModelConfig {
        default_model: Some("gpt-4".to_string()),
        temperature: None,
        system_prompt: Some("You are a pirate.".to_string()),
    };

    base_config.merge(&override_config);

    assert_eq!(base_config.default_model, Some("gpt-4".to_string()));
    assert_eq!(base_config.temperature, Some(0.8)); // Unchanged
    assert_eq!(base_config.system_prompt, Some("You are a pirate.".to_string()));
}
