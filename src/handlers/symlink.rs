use crate::state::AppState;
use crate::vfs::node::FsNode;
use std::path::PathBuf;

/// Handles the `readlink` FUSE operation.
/// Returns the target path for a symlink `FsNode`.
pub fn handle_readlink(_node: &FsNode, _state: &AppState) -> Result<PathBuf, libc::c_int> {
    unimplemented!()
}
