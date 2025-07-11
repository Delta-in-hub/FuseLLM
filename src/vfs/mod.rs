pub mod node;

use crate::state::AppState;
use node::FsNode;
use std::path::Path;

/// Resolves a filesystem path into a structured `FsNode`.
///
/// This is the central "router" of the VFS. It parses the path components
/// and determines which logical entity (file, directory, etc.) is being
/// accessed. It consults the `AppState` to validate dynamic path segments
/// like conversation IDs.
pub fn resolve_path(_path: &Path, _state: &AppState) -> FsNode {
    unimplemented!()
}
