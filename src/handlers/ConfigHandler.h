#pragma once
#include "../config/ConfigManager.h"
#include "../services/LLMClient.h"
#include "BaseHandler.h"
#include <vector>

namespace fusellm {
/**
 * @class ConfigHandler
 * @brief Handles read/write operations for configuration files under /config.
 *
 * It acts as a VFS layer on top of the ConfigManager, allowing users to
 * 'cat' and 'echo' to virtual TOML files to inspect and update configurations.
 */
class ConfigHandler : public BaseHandler {
  public:
    explicit ConfigHandler(ConfigManager &config, const LLMClient &client);

    int getattr(const char *path, struct stat *stbuf,
                struct fuse_file_info *fi) override;

    int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags) override;

    int open(const char *path, struct fuse_file_info *fi) override;

    int read(const char *path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) override;

    int write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) override;

  private:
    ConfigManager &default_config;
    std::vector<std::string> model_list_;

    std::unordered_map<std::string, std::string> models_config;
    std::mutex mtx_;
};
} // namespace fusellm