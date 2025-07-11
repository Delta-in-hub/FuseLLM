use crate::config::Config;
use serde::{Deserialize, Serialize};

/// Represents the role of a message in the conversation history.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum MessageRole {
    System,
    User,
    Assistant,
}

/// A single message in a conversation, with a role and content.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message {
    pub role: MessageRole,
    pub content: String,
}

/// Configuration specific to a single conversation, overriding global settings.
#[derive(Debug, Clone)]
pub struct ConversationConfig {
    /// The model used for this conversation (e.g., "gpt-4").
    pub model: String,
    /// The system prompt/role for this conversation.
    pub system_prompt: String,
    /// TOML value representing specific settings like temperature.
    pub settings: Option<toml::Value>,
}

/// Represents the complete state of a single, isolated conversation.
#[derive(Debug, Clone)]
pub struct Conversation {
    /// The unique identifier for this conversation (e.g., "my-project-chat").
    pub id: String,
    /// The complete chat history.
    pub history: Vec<Message>,
    /// The temporary, non-historical context for the next prompt.
    pub context: String,
    /// The specific configuration for this conversation.
    pub config: ConversationConfig,
    /// The most recent response from the LLM. Read by `cat prompt`.
    pub latest_response: String,
    /// A flag to indicate if a prompt is currently being processed.
    pub is_processing: bool,
}

impl Conversation {
    /// Creates a new conversation with a given ID, inheriting from global config.
    pub fn new(_id: String, _global_config: &Config) -> Self {
        unimplemented!()
    }

    /// Formats the entire conversation history into a human-readable string.
    pub fn format_history(&self) -> String {
        unimplemented!()
    }
}
