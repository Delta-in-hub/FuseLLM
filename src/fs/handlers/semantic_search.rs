//! # Handler for the Semantic Search Directory (`/semantic_search`)

use fuser::{FileAttr, FileType, ReplyAttr, ReplyData, ReplyDirectory, ReplyEntry, Request};
use libc::{ENOENT, EPERM};
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime};

use crate::fs::constants as C;
use crate::state::FilesystemState;

const TTL: Duration = Duration::from_secs(1);

/// Represents the `/semantic_search` directory.
pub struct SemanticSearchDir;

impl SemanticSearchDir {
    pub fn getattr(req: &Request<'_>) -> FileAttr {
        FileAttr {
            ino: C::INO_SEMANTIC_SEARCH,
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
        // TODO: List actual search indexes from state.
        reply.ok();
    }

    pub fn mkdir(id: &str, state: &Arc<Mutex<FilesystemState>>, reply: ReplyEntry) {
        // TODO: Create a new search index in the state.
        reply.error(EPERM); // Placeholder
    }
}

/// Represents a single search index directory, e.g., `/semantic_search/my_docs`.
pub struct SearchIndexDir;

impl SearchIndexDir {
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
        let entries = vec![
            (ino, FileType::Directory, "."),
            (ino, FileType::Directory, ".."),
            (ino + C::INO_SEMANTIC_INDEX_CORPUS_DIR_BASE, FileType::Directory, C::CORPUS_DIRNAME),
            (ino + C::INO_SEMANTIC_INDEX_QUERY_BASE, FileType::RegularFile, C::QUERY_FILENAME),
        ];
        // TODO: readdir logic
        reply.ok();
    }

    pub fn rmdir(id: &str, state: &Arc<Mutex<FilesystemState>>, reply: fuser::ReplyEmpty) {
        // TODO: Remove search index from state.
        reply.error(EPERM); // Placeholder
    }
}

/// Represents the `query` file or a file in the `corpus` directory.
pub struct SearchIndexFile;

impl SearchIndexFile {
    pub fn getattr(ino: u64, req: &Request<'_>, state: &Arc<Mutex<FilesystemState>>) -> FileAttr {
        // TODO: Get real size.
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
        // TODO: Implement read for query file.
        reply.data(b"");
    }

    pub fn write(reply: fuser::ReplyWrite) {
        // TODO: Implement write for query file and corpus files.
        reply.written(0);
    }

    pub fn create(reply: fuser::ReplyCreate) {
        // TODO: Implement create for corpus files.
        reply.error(EPERM);
    }
}
