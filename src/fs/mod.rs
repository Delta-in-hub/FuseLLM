//! # FUSE Filesystem Implementation
//!
//! This module contains the main `FuseLlm` struct and the primary
//! `fuser::Filesystem` trait implementation, which dispatches requests
//! to the appropriate handlers based on the inode.

use fuser::{
    FileAttr, Filesystem, KernelConfig, ReplyAttr, ReplyBmap, ReplyCreate, ReplyData,
    ReplyDirectory, ReplyEmpty, ReplyEntry, ReplyLock, ReplyLseek, ReplyOpen, ReplyStatfs,
    ReplyWrite, ReplyXattr, Request, TimeOrNow,
};
use libc::{c_int, ENOENT, ENOSYS};
use std::ffi::OsStr;
use std::path::Path;
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime};

use crate::state::FilesystemState;

const TTL: Duration = Duration::from_secs(1); // 1 second TTL

pub mod constants;
pub mod handlers;

/// The main struct representing our filesystem.
#[derive(Debug)]
pub struct FuseLlm {
    /// The shared, mutable state of the filesystem.
    pub state: Arc<Mutex<FilesystemState>>,
}

impl Filesystem for FuseLlm {
    fn lookup(&mut self, _req: &Request<'_>, parent: u64, name: &std::ffi::OsStr, reply: ReplyEntry) {
        // This is a simplified lookup. A real implementation would delegate to handlers.
        // For now, we just reply with ENOENT (No such file or directory) for everything.
        println!("lookup(parent={}, name={:?})", parent, name);
        reply.error(ENOENT);
    }

    fn getattr(&mut self, _req: &Request<'_>, ino: u64, _fh: Option<u64>, reply: ReplyAttr) {
        // This is a simplified getattr. A real implementation would delegate to handlers.
        println!("getattr(ino={})", ino);
        let attr = FileAttr {
            ino: constants::INO_ROOT,
            size: 0,
            blocks: 0,
            atime: SystemTime::now(),
            mtime: SystemTime::now(),
            ctime: SystemTime::now(),
            crtime: SystemTime::now(),
            kind: fuser::FileType::Directory,
            perm: 0o755,
            nlink: 2,
            uid: _req.uid(),
            gid: _req.gid(),
            rdev: 0,
            flags: 0,
            blksize: 512,
        };
        if ino == constants::INO_ROOT {
            reply.attr(&TTL, &attr);
        } else {
            reply.error(ENOENT);
        }
    }

    fn readdir(
        &mut self,
        _req: &Request<'_>,
        ino: u64,
        _fh: u64,
        offset: i64,
        mut reply: ReplyDirectory,
    ) {
        println!("readdir(ino={}, offset={})", ino, offset);
        if ino != constants::INO_ROOT {
            reply.error(ENOENT);
            return;
        }

        let entries = vec![
            (constants::INO_ROOT, fuser::FileType::Directory, "."),
            (constants::INO_ROOT, fuser::FileType::Directory, ".."), // This is not quite right, parent of root is root
            (constants::INO_MODELS, fuser::FileType::Directory, "models"),
            (constants::INO_CONFIG, fuser::FileType::Directory, "config"),
            (constants::INO_CONVERSATIONS, fuser::FileType::Directory, "conversations"),
            (constants::INO_SEMANTIC_SEARCH, fuser::FileType::Directory, "semantic_search"),
        ];

        for (i, (inode, kind, name)) in entries.iter().enumerate().skip(offset as usize) {
            if reply.add(*inode, (i + 1) as i64, *kind, name) {
                break; // Buffer is full
            }
        }
        reply.ok();
    }

    fn init(&mut self, _req: &Request<'_>, _config: &mut KernelConfig) -> Result<(), c_int> {
        println!("init");
        Ok(())
    }

    fn destroy(&mut self) {
        println!("destroy");
    }

    fn setattr(
        &mut self,
        _req: &Request<'_>,
        ino: u64,
        _mode: Option<u32>,
        _uid: Option<u32>,
        _gid: Option<u32>,
        _size: Option<u64>,
        _atime: Option<TimeOrNow>,
        _mtime: Option<TimeOrNow>,
        _ctime: Option<SystemTime>,
        _fh: Option<u64>,
        _crtime: Option<SystemTime>,
        _chgtime: Option<SystemTime>,
        _bkuptime: Option<SystemTime>,
        _flags: Option<u32>,
        reply: ReplyAttr,
    ) {
        println!("setattr(ino={})", ino);
        reply.error(ENOSYS);
    }

    fn mkdir(
        &mut self,
        _req: &Request<'_>,
        parent: u64,
        name: &OsStr,
        _mode: u32,
        _umask: u32,
        reply: ReplyEntry,
    ) {
        println!("mkdir(parent={}, name={:?})", parent, name);
        reply.error(ENOSYS);
    }

    fn unlink(&mut self, _req: &Request<'_>, parent: u64, name: &OsStr, reply: ReplyEmpty) {
        println!("unlink(parent={}, name={:?})", parent, name);
        reply.error(ENOSYS);
    }

    fn rmdir(&mut self, _req: &Request<'_>, parent: u64, name: &OsStr, reply: ReplyEmpty) {
        println!("rmdir(parent={}, name={:?})", parent, name);
        reply.error(ENOSYS);
    }

    fn rename(
        &mut self,
        _req: &Request<'_>,
        parent: u64,
        name: &OsStr,
        newparent: u64,
        newname: &OsStr,
        _flags: u32,
        reply: ReplyEmpty,
    ) {
        println!(
            "rename(parent={}, name={:?}, newparent={}, newname={:?})",
            parent,
            name,
            newparent,
            newname
        );
        reply.error(ENOSYS);
    }

    fn open(&mut self, _req: &Request<'_>, ino: u64, _flags: i32, reply: ReplyOpen) {
        println!("open(ino={})", ino);
        reply.opened(0, 0);
    }

    fn read(
        &mut self,
        _req: &Request<'_>,
        ino: u64,
        _fh: u64,
        offset: i64,
        _size: u32,
        _flags: i32,
        _lock_owner: Option<u64>,
        reply: ReplyData,
    ) {
        println!("read(ino={}, offset={})", ino, offset);
        reply.error(ENOSYS);
    }

    fn write(
        &mut self,
        _req: &Request<'_>,
        ino: u64,
        _fh: u64,
        offset: i64,
        _data: &[u8],
        _write_flags: u32,
        _flags: i32,
        _lock_owner: Option<u64>,
        reply: ReplyWrite,
    ) {
        println!("write(ino={}, offset={})", ino, offset);
        reply.error(ENOSYS);
    }

    fn release(
        &mut self,
        _req: &Request<'_>,
        ino: u64,
        _fh: u64,
        _flags: i32,
        _lock_owner: Option<u64>,
        _flush: bool,
        reply: ReplyEmpty,
    ) {
        println!("release(ino={})", ino);
        reply.ok();
    }

    fn opendir(&mut self, _req: &Request<'_>, ino: u64, _flags: i32, reply: ReplyOpen) {
        println!("opendir(ino={})", ino);
        reply.opened(0, 0);
    }

    fn releasedir(
        &mut self,
        _req: &Request<'_>,
        ino: u64,
        _fh: u64,
        _flags: i32,
        reply: ReplyEmpty,
    ) {
        println!("releasedir(ino={})", ino);
        reply.ok();
    }

    fn create(
        &mut self,
        _req: &Request<'_>,
        parent: u64,
        name: &OsStr,
        _mode: u32,
        _umask: u32,
        _flags: i32,
        reply: ReplyCreate,
    ) {
        println!("create(parent={}, name={:?})", parent, name);
        reply.error(ENOSYS);
    }
}
