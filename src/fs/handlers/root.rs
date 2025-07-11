//! # Handler for the Root Directory (`/`)

use fuser::{FileAttr, FileType, ReplyAttr, ReplyDirectory, ReplyEntry, Request};
use libc::ENOENT;
use std::time::{Duration, SystemTime};

use crate::fs::constants as C;

const TTL: Duration = Duration::from_secs(1);

/// Structure representing the root directory.
pub struct RootDirectory;

impl RootDirectory {
    /// Returns the attributes of the root directory.
    pub fn getattr(req: &Request<'_>) -> FileAttr {
        FileAttr {
            ino: C::INO_ROOT,
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

    /// Looks up an entry in the root directory.
    pub fn lookup(name: &str, reply: ReplyEntry) {
        let inode = match name {
            "models" => C::INO_MODELS,
            "config" => C::INO_CONFIG,
            "conversations" => C::INO_CONVERSATIONS,
            "semantic_search" => C::INO_SEMANTIC_SEARCH,
            _ => {
                reply.error(ENOENT);
                return;
            }
        };
        // For now, we'll just return placeholder attributes.
        // A real implementation would fetch them from the respective handlers.
        let attr = FileAttr {
            ino: inode,
            size: 0,
            blocks: 0,
            atime: SystemTime::now(),
            mtime: SystemTime::now(),
            ctime: SystemTime::now(),
            crtime: SystemTime::now(),
            kind: FileType::Directory,
            perm: 0o755,
            nlink: 2,
            uid: 0, // Should be req.uid()
            gid: 0, // Should be req.gid()
            rdev: 0,
            flags: 0,
            blksize: 512,
        };
        reply.entry(&TTL, &attr, 0);
    }

    /// Reads the contents of the root directory.
    pub fn readdir(offset: i64, mut reply: ReplyDirectory) {
        let entries = vec![
            (C::INO_ROOT, FileType::Directory, "."),
            (C::INO_ROOT, FileType::Directory, ".."),
            (C::INO_MODELS, FileType::Directory, "models"),
            (C::INO_CONFIG, FileType::Directory, "config"),
            (C::INO_CONVERSATIONS, FileType::Directory, "conversations"),
            (C::INO_SEMANTIC_SEARCH, FileType::Directory, "semantic_search"),
        ];

        for (i, (inode, kind, name)) in entries.into_iter().enumerate().skip(offset as usize) {
            if reply.add(inode, (i + 1) as i64, kind, name) {
                break; // Buffer is full
            }
        }
        reply.ok();
    }
}
