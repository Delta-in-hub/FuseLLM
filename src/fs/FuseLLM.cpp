#include "FuseLLM.h"
#include "../../external/Fusepp/Fuse-impl.h" // Can't remove this
#include "../handlers/ConfigHandler.h"
#include "../handlers/ConversationsHandler.h"
#include "../handlers/ModelsHandler.h"
#include "../handlers/RootHandler.h"
#include "../handlers/SemanticSearchHandler.h"
#include "PathParser.h"
#include <cerrno> // For error codes like ENOENT
#include <spdlog/spdlog.h>

namespace fusellm {

std::unordered_map<PathType, std::unique_ptr<BaseHandler>> FuseLLM::handlers;

FuseLLM &FuseLLM::getInstance(ConfigManager &config) {
    static FuseLLM instance(config);
    return instance;
}

FuseLLM::FuseLLM(ConfigManager &config)
    : global_config(config), session_manager(config), llm_client(config),
      zmq_client() {
    SPDLOG_INFO("Initializing FuseLLM filesystem components...");

    // TODO: Connect zmq client
    zmq_client.connect(config.semantic_search_service_url_);

    // Map path types to their corresponding handlers.
    handlers[PathType::Root] = std::make_unique<RootHandler>();
    handlers[PathType::Models] = std::make_unique<ModelsHandler>(
        llm_client, global_config, session_manager);

    handlers[PathType::Config] =
        std::make_unique<ConfigHandler>(global_config, llm_client);

    handlers[PathType::Conversations] = std::make_unique<ConversationsHandler>(
        session_manager, llm_client, global_config);

    handlers[PathType::SemanticSearch] =
        std::make_unique<SemanticSearchHandler>(zmq_client);

    SPDLOG_INFO("All handlers initialized and mapped.");
}

BaseHandler *FuseLLM::get_handler(std::string_view path) {
    PathType parsed_path = PathParser::parse(path);
    auto it = handlers.find(parsed_path);
    if (it != handlers.end()) {
        return it->second.get();
    }
    SPDLOG_WARN("No handler found for path '{}' (type: {})", path,
                static_cast<int>(parsed_path));
    return nullptr;
}

// --- FUSE Callback Implementations ---
// Each callback simply finds the correct handler and delegates the call.

int FuseLLM::getattr(const char *path, struct stat *stbuf,
                     struct fuse_file_info *fi) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -ENOENT; // No such file or directory
    return handler->getattr(path, stbuf, fi);
}

int FuseLLM::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags flags) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -ENOENT;
    return handler->readdir(path, buf, filler, offset, fi, flags);
}

int FuseLLM::open(const char *path, struct fuse_file_info *fi) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -ENOENT;
    return handler->open(path, fi);
}

int FuseLLM::read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -ENOENT;
    return handler->read(path, buf, size, offset, fi);
}

int FuseLLM::write(const char *path, const char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -ENOENT;
    return handler->write(path, buf, size, offset, fi);
}

int FuseLLM::mkdir(const char *path, mode_t mode) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -EPERM; // Operation not permitted
    return handler->mkdir(path, mode);
}

int FuseLLM::rmdir(const char *path) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -ENOENT;
    return handler->rmdir(path);
}

int FuseLLM::unlink(const char *path) {
    BaseHandler *handler = get_handler(path);
    if (!handler)
        return -ENOENT;
    return handler->unlink(path);
}

} // namespace fusellm