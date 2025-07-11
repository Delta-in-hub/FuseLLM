//! # Filesystem Constants
//!
//! This module defines the static inode numbers for the filesystem.
//! Having them in one place makes it easier to manage and avoid collisions.

// The root inode is always 1.
pub const INO_ROOT: u64 = 1;

// Top-level directories (2-99)
pub const INO_MODELS: u64 = 2;
pub const INO_CONFIG: u64 = 3;
pub const INO_CONVERSATIONS: u64 = 4;
pub const INO_SEMANTIC_SEARCH: u64 = 5;

// Config directory contents (100-199)
pub const INO_CONFIG_SETTINGS: u64 = 100;
pub const INO_CONFIG_MODELS_DIR: u64 = 101;

// Conversations directory contents (200-299)
pub const INO_CONVERSATIONS_LATEST: u64 = 200;

// Semantic Search directory contents (300-399)
pub const INO_SEMANTIC_SEARCH_DEFAULT: u64 = 300;

// Dynamic inode ranges.
// These are the starting points. The actual inode will be this base + some unique ID.

// Models available under /models (e.g., gpt-4)
pub const INO_MODELS_BASE: u64 = 1_000;

// Specific model config dirs under /config/models/
pub const INO_CONFIG_MODEL_DIR_BASE: u64 = 2_000;
// settings.toml within a specific model config dir
pub const INO_CONFIG_MODEL_SETTINGS_BASE: u64 = 3_000;

// Conversation session directories under /conversations/
pub const INO_CONVERSATION_DIR_BASE: u64 = 10_000;
// Files within a conversation directory
pub const INO_CONVERSATION_PROMPT_BASE: u64 = 20_000;
pub const INO_CONVERSATION_HISTORY_BASE: u64 = 30_000;
pub const INO_CONVERSATION_CONTEXT_BASE: u64 = 40_000;
pub const INO_CONVERSATION_CONFIG_DIR_BASE: u64 = 50_000;
pub const INO_CONVERSATION_CONFIG_MODEL_BASE: u64 = 60_000;
pub const INO_CONVERSATION_CONFIG_SYSTEM_PROMPT_BASE: u64 = 70_000;
pub const INO_CONVERSATION_CONFIG_SETTINGS_BASE: u64 = 80_000;

// Semantic search index dirs under /semantic_search/
pub const INO_SEMANTIC_INDEX_DIR_BASE: u64 = 100_000;
// Files/dirs within a semantic search index directory
pub const INO_SEMANTIC_INDEX_CORPUS_DIR_BASE: u64 = 200_000;
pub const INO_SEMANTIC_INDEX_QUERY_BASE: u64 = 300_000;

// Corpus files are highly dynamic, we'll need another system for them.


// File and directory names
pub const PROMPT_FILENAME: &str = "prompt";
pub const HISTORY_FILENAME: &str = "history";
pub const CONTEXT_FILENAME: &str = "context";
pub const CONFIG_FILENAME: &str = "config";
pub const MODEL_FILENAME: &str = "model";
pub const SYSTEM_PROMPT_FILENAME: &str = "system_prompt";
pub const SETTINGS_FILENAME: &str = "settings.toml";
pub const CORPUS_DIRNAME: &str = "corpus";
pub const QUERY_FILENAME: &str = "query";
