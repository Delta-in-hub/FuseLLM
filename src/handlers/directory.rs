use crate::state::{AppState, conversation::Conversation};
use crate::vfs::node::FsNode;
use fuse::FileType;

/// Represents an entry in a directory listing.
pub struct DirEntry {
    pub inode: u64,
    pub name: String,
    pub file_type: FileType,
}

/// Handles the `readdir` FUSE operation.
/// Returns a list of directory entries for a given `FsNode` representing a directory.
pub fn handle_readdir(_node: &FsNode, _state: &AppState) -> Result<Vec<DirEntry>, libc::c_int> {
    unimplemented!()
}

/// Handles the `mkdir` FUSE operation.
/// Creates a new conversation or search index.
pub fn handle_mkdir(_parent_node: &FsNode, _name: &str, _state: &mut AppState) -> Result<DirEntry, libc::c_int> {
    unimplemented!()
}

/// Handles the `rmdir` FUSE operation.
/// Deletes a conversation or search index.
pub fn handle_rmdir(_parent_node: &FsNode, _name: &str, _state: &mut AppState) -> Result<(), libc::c_int> {
    unimplemented!()
}
