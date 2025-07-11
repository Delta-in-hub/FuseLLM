use crate::state::AppState;
use crate::vfs::node::FsNode;

/// Handles the `write` FUSE operation for all writable virtual files.
/// This is where prompts are sent, contexts are set, and configs are updated.
///
/// Returns the number of bytes written.
pub async fn handle_write(
    _node: &FsNode,
    _data: &[u8],
    _state: &mut AppState,
    _llm_service: &crate::services::llm_api::LlmService,
) -> Result<usize, libc::c_int> {
    unimplemented!()
}

/// Handles the `create` and subsequent `write` operations for corpus files.
pub async fn handle_corpus_write(
    _node: &FsNode,
    _data: &[u8],
    _state: &mut AppState,
    _search_client: &mut crate::services::search_client::SearchClient,
) -> Result<usize, libc::c_int> {
    unimplemented!()
}


/// Handles the `unlink` operation for corpus files.
pub fn handle_corpus_unlink(
    _node: &FsNode,
    _state: &mut AppState,
    _search_client: &mut crate::services::search_client::SearchClient,
) -> Result<(), libc::c_int> {
    unimplemented!()
}
