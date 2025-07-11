// src/fs/FuseLLM.h
#pragma once

#include "../../external/Fusepp/Fuse.h"
#include "PathParser.h"
#include "../state/SessionManager.h"
#include "../config/ConfigManager.h"
#include "../services/LLMClient.h"
#include "../services/ZmqClient.h"
#include "../handlers/BaseHandler.h"
#include <memory>
#include <unordered_map>

namespace fusellm {

// FUSE 文件系统的核心实现，继承自 fusepp
class FuseLLM : public Fusepp::Fuse<FuseLLM> {
public:
    FuseLLM();

    // FUSE 回调函数，将作为 FUSE 操作的入口点
    // fusepp 通过 CRTP (Curiously Recurring Template Pattern) 调用这些静态方法
    static int getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
    static int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
    static int open(const char* path, struct fuse_file_info* fi);
    static int read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
    static int write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
    static int mkdir(const char* path, mode_t mode);
    static int rmdir(const char* path);
    static int symlink(const char* from, const char* to);
    static int readlink(const char* path, char* buf, size_t size);
    // ... 其他 FUSE 操作

private:
    // 根据路径将请求分派给正确的 Handler
    static BaseHandler* get_handler(const std::string& path);

    // 静态成员来存储文件系统的状态和服务的客户端
    // 这是因为 FUSE 回调是静态的
    static std::unique_ptr<SessionManager> session_manager_;
    static std::unique_ptr<ConfigManager> config_manager_;
    static std::unique_ptr<LLMClient> llm_client_;
    static std::unique_ptr<ZmqClient> zmq_client_;

    // 存储不同路径类型的处理器
    static std::unordered_map<PathType, std::unique_ptr<BaseHandler>> handlers_;
};

} // namespace fusellm
