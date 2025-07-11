//! # Handler for the Config Directory (`/config`)

use fuser::{FileAttr, FileType, ReplyAttr, ReplyData, ReplyDirectory, ReplyEntry, Request};
use libc::{ENOENT, EPERM};
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime};

use crate::fs::constants as C;
use crate::state::FilesystemState;

const TTL: Duration = Duration::from_secs(1);

/// Represents the `/config` directory.
pub struct ConfigDir;

impl ConfigDir {
    pub fn getattr(req: &Request<'_>) -> FileAttr {
        FileAttr {
            ino: C::INO_CONFIG,
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

    pub fn readdir(offset: i64, mut reply: ReplyDirectory) {
        let entries = vec![
            (C::INO_CONFIG, FileType::Directory, "."),
            (C::INO_CONFIG, FileType::Directory, ".."),
            (C::INO_CONFIG_SETTINGS, FileType::RegularFile, C::SETTINGS_FILENAME),
            (C::INO_CONFIG_MODELS_DIR, FileType::Directory, "models"),
        ];
        // TODO: Add readdir logic
        reply.ok();
    }
}

/// Represents a config file like `settings.toml`.
pub struct ConfigFile;

impl ConfigFile {
    pub fn getattr(ino: u64, req: &Request<'_>, state: &Arc<Mutex<FilesystemState>>) -> FileAttr {
        // TODO: Get real size from serialized config.
        FileAttr {
            ino,
            size: 0,
            blocks: 0,
            atime: SystemTime::now(),
            mtime: SystemTime::now(),
            ctime: SystemTime::now(),
            crtime: SystemTime::now(),
            kind: FileType::RegularFile,
            perm: 0o644,
            nlink: 1,
            uid: req.uid(),
            gid: req.gid(),
            rdev: 0,
            flags: 0,
            blksize: 512,
        }
    }

    pub fn read(reply: ReplyData) {
        // TODO: Read and serialize the relevant config from state.
        reply.data(b"");
    }

    pub fn write(reply: fuser::ReplyWrite) {
        // TODO: Deserialize, validate, and update the config in state.
        reply.written(0);
    }
}
