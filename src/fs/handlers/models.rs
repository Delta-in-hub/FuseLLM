//! # Handler for the Models Directory (`/models`)

use fuser::{FileAttr, FileType, ReplyAttr, ReplyData, ReplyDirectory, ReplyEntry, Request};
use libc::{ENOENT, EPERM};
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime};

use crate::fs::constants as C;
use crate::state::FilesystemState;

const TTL: Duration = Duration::from_secs(1);

/// Represents the `/models` directory.
pub struct ModelsDir;

impl ModelsDir {
    /// Returns the attributes of the `/models` directory.
    pub fn getattr(req: &Request<'_>) -> FileAttr {
        FileAttr {
            ino: C::INO_MODELS,
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

    /// Reads the contents of the `/models` directory.
    pub fn readdir(state: &Arc<Mutex<FilesystemState>>, offset: i64, mut reply: ReplyDirectory) {
        // In a real implementation, this would list models from the config.
        let entries = vec![
            (C::INO_MODELS, FileType::Directory, "."),
            (C::INO_MODELS, FileType::Directory, ".."),
            // Placeholder for a model
            (C::INO_MODELS_BASE + 1, FileType::RegularFile, "gpt-4"),
        ];

        for (i, (inode, kind, name)) in entries.into_iter().enumerate().skip(offset as usize) {
            if reply.add(inode, (i + 1) as i64, kind, name) {
                break;
            }
        }
        reply.ok();
    }
}

/// Represents a model file like `/models/gpt-4`.
pub struct ModelFile;

impl ModelFile {
    /// Returns the attributes of a model file.
    pub fn getattr(ino: u64, req: &Request<'_>, state: &Arc<Mutex<FilesystemState>>) -> FileAttr {
        // In a real implementation, size would be the length of the last response.
        FileAttr {
            ino,
            size: 0,
            blocks: 0,
            atime: SystemTime::now(),
            mtime: SystemTime::now(),
            ctime: SystemTime::now(),
            crtime: SystemTime::now(),
            kind: FileType::RegularFile,
            perm: 0o644, // Read for all, write for owner
            nlink: 1,
            uid: req.uid(),
            gid: req.gid(),
            rdev: 0,
            flags: 0,
            blksize: 512,
        }
    }

    /// Reads the content of a model file (the last response).
    pub fn read(
        _ino: u64,
        _offset: i64,
        _size: u32,
        _state: &Arc<Mutex<FilesystemState>>,
        reply: ReplyData,
    ) {
        // TODO: Get the last response for this model from the state.
        reply.data(b"");
    }

    /// Writes to a model file, triggering a new LLM query.
    pub fn write(
        _ino: u64,
        _offset: i64,
        _data: &[u8],
        _state: &Arc<Mutex<FilesystemState>>,
        reply: fuser::ReplyWrite,
    ) {
        // TODO: Trigger LLM API call.
        // For now, just accept the write.
        reply.written(0);
    }
}
