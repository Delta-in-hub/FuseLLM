use crate::state::AppState;
use crate::vfs::node::FsNode;

/// Handles the `read` FUSE operation for all virtual files.
/// Returns the byte content of a file represented by `FsNode`.
pub fn handle_read(_node: &FsNode, _state: &AppState) -> Result<Vec<u8>, libc::c_int> {
    unimplemented!()
}
