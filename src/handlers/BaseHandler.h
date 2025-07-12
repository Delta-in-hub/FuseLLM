// src/handlers/BaseHandler.h
#pragma once

#include "../../external/Fusepp/Fuse.h"
#include <cerrno>

namespace fusellm {

// 所有 Handler 的基类接口 (可选，但有助于统一接口)
// fusepp 本身不强制要求，但这样做可以使我们的 FuseLLM 类更整洁
class BaseHandler {
  public:
    virtual ~BaseHandler() = default;

    // 对于未实现的操作，默认返回 "Function not implemented"
    // 注意：我们将 "= 0" 替换为了一个默认的函数体 "{ return -ENOSYS; }"
    virtual int getattr(const char *path, struct stat *stbuf,
                        struct fuse_file_info *fi) {
        (void)path;
        (void)stbuf;
        (void)fi; // 避免 "unused parameter" 警告
        return -ENOSYS;
    }

    virtual int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags) {
        (void)path;
        (void)buf;
        (void)filler;
        (void)offset;
        (void)fi;
        (void)flags;
        return -ENOSYS;
    }

    virtual int open(const char *path, struct fuse_file_info *fi) {
        (void)path;
        (void)fi;
        return -ENOSYS;
    }

    virtual int read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
        (void)path;
        (void)buf;
        (void)size;
        (void)offset;
        (void)fi;
        return -ENOSYS;
    }

    virtual int write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi) {
        (void)path;
        (void)buf;
        (void)size;
        (void)offset;
        (void)fi;
        return -ENOSYS;
    }

    virtual int mkdir(const char *path, mode_t mode) {
        (void)path;
        (void)mode;
        return -ENOSYS;
    }

    virtual int rmdir(const char *path) {
        (void)path;
        return -ENOSYS;
    }
    virtual int unlink(const char *path) {
        (void)path;
        return -ENOSYS;
    }
    virtual int mknod(const char *path, mode_t mode, dev_t rdev) {
        (void)path;
        (void)mode;
        (void)rdev;
        return -ENOSYS;
    }
    // ... 其他 FUSE 操作也可以提供默认实现
};

} // namespace fusellm
