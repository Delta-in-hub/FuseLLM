// src/fs/FuseLLM.cpp
#include "FuseLLM.h"
#include "../../external/Fusepp/Fuse-impl.h"
// FUSE 回调的实现和到具体 Handler 的分派逻辑将在这里完成

#include <cerrno> // For ENOSYS

namespace fusellm {

// 静态成员变量的定义
std::unique_ptr<SessionManager> FuseLLM::session_manager_;
std::unique_ptr<ConfigManager> FuseLLM::config_manager_;
std::unique_ptr<LLMClient> FuseLLM::llm_client_;
std::unique_ptr<ZmqClient> FuseLLM::zmq_client_;
std::unordered_map<PathType, std::unique_ptr<BaseHandler>> FuseLLM::handlers_;

// 构造函数的定义
FuseLLM::FuseLLM() {
    // 在这里可以进行初始化，例如加载配置、初始化客户端等
    config_manager_ = std::make_unique<ConfigManager>();
    session_manager_ = std::make_unique<SessionManager>();
    // 其他初始化...
}

// FUSE 回调函数的存根实现
int FuseLLM::getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi) {
    return -ENOSYS; // 功能未实现
}

int FuseLLM::readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    return -ENOSYS;
}

int FuseLLM::open(const char* path, struct fuse_file_info* fi) {
    return -ENOSYS;
}

int FuseLLM::read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    return -ENOSYS;
}

int FuseLLM::write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    return -ENOSYS;
}

int FuseLLM::mkdir(const char* path, mode_t mode) {
    return -ENOSYS;
}

int FuseLLM::rmdir(const char* path) {
    return -ENOSYS;
}

int FuseLLM::symlink(const char* from, const char* to) {
    return -ENOSYS;
}

int FuseLLM::readlink(const char* path, char* buf, size_t size) {
    return -ENOSYS;
}

BaseHandler* FuseLLM::get_handler(const std::string& path) {
    return nullptr;
}

} // namespace fusellm
