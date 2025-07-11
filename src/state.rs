use crate::config::{GlobalConfig, ModelConfig};
use anyhow::Result;
use std::collections::HashMap;

/// A unique identifier for a conversation session.
pub type SessionId = String;
/// A unique identifier for a semantic search index.
pub type IndexId = String;
/// A unique identifier for a model.
pub type ModelId = String;

/// The central, shared state of the entire filesystem.
/// This struct is wrapped in an Arc<Mutex<...>> to allow for safe, concurrent access
/// from multiple FUSE threads.
#[derive(Debug)]
pub struct FilesystemState {
    /// Global configuration, loaded at startup.
    pub config: GlobalConfig,
    /// A map of all active conversation sessions.
    pub conversations: HashMap<SessionId, Conversation>,
    /// A map of all active semantic search indexes.
    pub search_indexes: HashMap<IndexId, SearchIndex>,
    /// A map to store the last response for stateless model queries.
    pub model_last_response: HashMap<ModelId, String>,
    /// The ID of the most recently used conversation.
    pub latest_conversation: Option<SessionId>,
}

/// Represents a single, stateful conversation.
#[derive(Debug, Clone)]
pub struct Conversation {
    pub id: SessionId,
    /// The complete history of the conversation.
    pub history: Vec<HistoryEntry>,
    /// Temporary, out-of-band context for the next prompt.
    pub context: Option<String>,
    /// The last response received from the LLM.
    pub last_response: Option<String>,
    /// Conversation-specific configuration, overriding global settings.
    pub config: ModelConfig,
}

/// A single entry in the conversation history.
#[derive(Debug, Clone)]
pub struct HistoryEntry {
    pub role: Role,
    pub content: String,
}

/// The role of a participant in the conversation.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Role {
    User,
    LLM,
}

/// Represents a semantic search index.
#[derive(Debug, Clone)]
pub struct SearchIndex {
    pub id: IndexId,
    /// A map of file paths within the corpus to their content.
    pub corpus: HashMap<String, String>,
    /// The last query result for this index.
    pub last_query_result: String,
}

impl FilesystemState {
    /// Creates a new, empty filesystem state with the given configuration.
    pub fn new(config: GlobalConfig) -> Self {
        Self {
            config,
            conversations: HashMap::new(),
            search_indexes: HashMap::new(),
            model_last_response: HashMap::new(),
            latest_conversation: None,
        }
    }

    /// Creates a new conversation and adds it to the state.
    pub fn create_conversation(&mut self, id: SessionId) -> Result<&mut Conversation> {
        if self.conversations.contains_key(&id) {
            return Err(anyhow::anyhow!("Conversation already exists"));
        }
        // Inherit from global model config
        let conversation_config = self.config.default_config.clone();
        let conversation = Conversation {
            id: id.clone(),
            history: Vec::new(),
            context: None,
            last_response: None,
            config: conversation_config,
        };
        self.conversations.insert(id.clone(), conversation);
        Ok(self.conversations.get_mut(&id).unwrap())
    }
}
