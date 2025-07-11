use crate::state::AppState;
use crate::vfs::node::FsNode;
use fuse::FileAttr;

/// Handles the `getattr` FUSE operation.
/// Returns the file attributes (permissions, size, type, etc.) for a given `FsNode`.
pub fn handle_getattr(_node: &FsNode, _state: &AppState) -> Result<FileAttr, libc::c_int> {
    unimplemented!()
}
