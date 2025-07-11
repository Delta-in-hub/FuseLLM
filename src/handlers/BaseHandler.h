// src/handlers/BaseHandler.h
#pragma once

#include <fuse.h>
#include <string>

namespace fusellm {

// 所有 Handler 的基类接口 (可选，但有助于统一接口)
// fusepp 本身不强制要求，但这样做可以使我们的 FuseLLM 类更整洁
class BaseHandler {
public:
    virtual ~BaseHandler() = default;

    virtual int getattr(const char* path, struct stat* stbuf) = 0;
    virtual int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) = 0;
    virtual int open(const char* path, struct fuse_file_info* fi) = 0;
    virtual int read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) = 0;
    virtual int write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) = 0;
    virtual int mkdir(const char* path, mode_t mode) = 0;
    virtual int rmdir(const char* path) = 0;
    // ... 其他需要实现的 FUSE 操作
};

} // namespace fusellm
