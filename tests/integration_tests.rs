//! # Integration Tests for FuseLLM

// Note: True integration tests would require mounting the filesystem and interacting
// with it using standard file operations. This is complex to set up in an automated test.
// These tests will be more like high-level unit tests that simulate interactions.

use std::sync::{Arc, Mutex};
use fusellm::config::GlobalConfig;
use fusellm::state::FilesystemState;

#[test]
fn test_conversation_creation() {
    let config = GlobalConfig::default();
    let mut state = FilesystemState::new(config);

    let session_id = "test_session".to_string();
    state.create_conversation(session_id.clone()).unwrap();

    assert!(state.conversations.contains_key(&session_id));
    let conv = state.conversations.get(&session_id).unwrap();
    assert_eq!(conv.id, session_id);
    assert!(conv.history.is_empty());
}
