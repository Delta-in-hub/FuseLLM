// src/fs/FuseLLM.h
#pragma once

#include "../../external/Fusepp/Fuse.h"
#include "../config/ConfigManager.h"
#include "../handlers/BaseHandler.h"
#include "../services/LLMClient.h"
#include "../services/ZmqClient.h"
#include "../state/SessionManager.h"
#include "PathParser.h"
#include <memory>
#include <unordered_map>

namespace fusellm {

// FUSE 文件系统的核心实现，继承自 fusepp
class FuseLLM : public Fusepp::Fuse<FuseLLM> {
  public:
    // 获取单例的静态方法
    static FuseLLM &getInstance(const ConfigManager &config);

    FuseLLM() = delete;
    FuseLLM(const FuseLLM &) = delete;
    FuseLLM(FuseLLM &&) = delete;
    FuseLLM &operator=(const FuseLLM &) = delete;
    FuseLLM &operator=(FuseLLM &&) = delete;

    // FUSE 回调函数，将作为 FUSE 操作的入口点
    // fusepp 通过 CRTP (Curiously Recurring Template Pattern) 调用这些静态方法
    static int getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi);
    static int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags);
    static int open(const char *path, struct fuse_file_info *fi);
    static int read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi);
    static int write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi);
    static int mkdir(const char *path, mode_t mode);
    static int rmdir(const char *path);
    // ... 其他 FUSE 操作

  private:
    explicit FuseLLM(const ConfigManager &config);
    ConfigManager global_config;
    // 根据路径将请求分派给正确的 Handler
    static BaseHandler *get_handler(std::string_view path);

    SessionManager session_manager;
    LLMClient llm_client;
    ZmqClient zmq_client;

    // 存储不同路径类型的处理器
    static std::unordered_map<PathType, std::unique_ptr<BaseHandler>> handlers;
};

} // namespace fusellm
