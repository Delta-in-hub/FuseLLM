//! # Handler for the Conversations Directory (`/conversations`)

use fuser::{FileAttr, FileType, ReplyAttr, ReplyData, ReplyDirectory, ReplyEntry, Request};
use libc::{ENOENT, EPERM};
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime};

use crate::fs::constants as C;
use crate::state::FilesystemState;

const TTL: Duration = Duration::from_secs(1);

/// Represents the `/conversations` directory.
pub struct ConversationsDir;

impl ConversationsDir {
    pub fn getattr(req: &Request<'_>) -> FileAttr {
        FileAttr {
            ino: C::INO_CONVERSATIONS,
            size: 0,
            blocks: 0,
            atime: SystemTime::now(),
            mtime: SystemTime::now(),
            ctime: SystemTime::now(),
            crtime: SystemTime::now(),
            kind: FileType::Directory,
            perm: 0o755,
            nlink: 2,
            uid: req.uid(),
            gid: req.gid(),
            rdev: 0,
            flags: 0,
            blksize: 512,
        }
    }

    pub fn readdir(state: &Arc<Mutex<FilesystemState>>, offset: i64, mut reply: ReplyDirectory) {
        let mut entries = vec![
            (C::INO_CONVERSATIONS, FileType::Directory, "."),
            (C::INO_CONVERSATIONS, FileType::Directory, ".."),
        ];
        // TODO: List actual conversations from state.
        reply.ok();
    }

    pub fn mkdir(id: &str, state: &Arc<Mutex<FilesystemState>>, reply: ReplyEntry) {
        // TODO: Create a new conversation in the state.
        reply.error(EPERM); // Placeholder
    }
}

/// Represents a single conversation directory, e.g., `/conversations/123`.
pub struct ConversationDir;

impl ConversationDir {
    pub fn getattr(ino: u64, req: &Request<'_>) -> FileAttr {
        FileAttr {
            ino,
            size: 0,
            blocks: 0,
            atime: SystemTime::now(),
            mtime: SystemTime::now(),
            ctime: SystemTime::now(),
            crtime: SystemTime::now(),
            kind: FileType::Directory,
            perm: 0o755,
            nlink: 2,
            uid: req.uid(),
            gid: req.gid(),
            rdev: 0,
            flags: 0,
            blksize: 512,
        }
    }

    pub fn readdir(ino: u64, offset: i64, mut reply: ReplyDirectory) {
        let mut entries = vec![
            (ino, FileType::Directory, "."),
            (ino, FileType::Directory, ".."),
            (ino + C::INO_CONVERSATION_PROMPT_BASE, FileType::RegularFile, C::PROMPT_FILENAME),
            (ino + C::INO_CONVERSATION_HISTORY_BASE, FileType::RegularFile, C::HISTORY_FILENAME),
            (ino + C::INO_CONVERSATION_CONTEXT_BASE, FileType::RegularFile, C::CONTEXT_FILENAME),
            (ino + C::INO_CONVERSATION_CONFIG_DIR_BASE, FileType::Directory, C::CONFIG_FILENAME),
        ];
        // TODO: Add logic for readdir
        reply.ok();
    }

    pub fn rmdir(id: &str, state: &Arc<Mutex<FilesystemState>>, reply: fuser::ReplyEmpty) {
        // TODO: Remove conversation from state.
        reply.error(EPERM); // Placeholder
    }
}

/// Represents one of the files inside a conversation directory (e.g., `prompt`).
pub struct ConversationFile;

impl ConversationFile {
    pub fn getattr(ino: u64, req: &Request<'_>, state: &Arc<Mutex<FilesystemState>>) -> FileAttr {
        // TODO: Get real size based on content.
        FileAttr {
            ino,
            size: 0,
            blocks: 0,
            atime: SystemTime::now(),
            mtime: SystemTime::now(),
            ctime: SystemTime::now(),
            crtime: SystemTime::now(),
            kind: FileType::RegularFile,
            perm: 0o644, // Default permissions
            nlink: 1,
            uid: req.uid(),
            gid: req.gid(),
            rdev: 0,
            flags: 0,
            blksize: 512,
        }
    }

    pub fn read(reply: ReplyData) {
        // TODO: Implement read logic for prompt, history, context.
        reply.data(b"");
    }

    pub fn write(reply: fuser::ReplyWrite) {
        // TODO: Implement write logic for prompt, context.
        reply.written(0);
    }
}
