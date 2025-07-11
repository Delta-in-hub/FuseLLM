pub mod conversation;
pub mod search_index;

use crate::config::Config;
use conversation::Conversation;
use search_index::SearchIndex;
use std::collections::HashMap;

/// `AppState` holds the entire live state of the filesystem.
///
/// This struct is wrapped in an `Arc<RwLock<>>` to allow safe, shared,
/// mutable access across all FUSE threads.
#[derive(Debug)]
pub struct AppState {
    /// The application's configuration, loaded at startup.
    pub config: Config,

    /// A map of session IDs to their corresponding `Conversation` state.
    pub conversations: HashMap<String, Conversation>,

    /// A map of index IDs to their corresponding `SearchIndex` state.
    pub search_indexes: HashMap<String, SearchIndex>,

    /// Tracks the ID of the most recently interacted-with conversation for the `latest` symlink.
    pub latest_conversation_id: Option<String>,

    /// Stores the last stateless reply for each model available under `/models`.
    pub model_last_response: HashMap<String, String>,
}

impl AppState {
    /// Creates a new, initial state from the application configuration.
    pub fn new(_config: Config) -> Self {
        unimplemented!()
    }
}
