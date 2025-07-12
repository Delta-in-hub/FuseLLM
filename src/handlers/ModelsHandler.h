#pragma once
#include "../config/ConfigManager.h"
#include "../services/LLMClient.h"
#include "../state/SessionManager.h"
#include "BaseHandler.h"
#include <mutex>
#include <unordered_map>

namespace fusellm {

/**
 * @class ModelsHandler
 * @brief Handles all operations under the /models directory.
 *
 * This includes listing available models, performing stateless queries via
 * read/write operations on model files, and managing the 'default' symlink.
 */
class ModelsHandler : public BaseHandler {
  public:
    explicit ModelsHandler(LLMClient &client, ConfigManager &config, SessionManager &sessions);

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
    LLMClient &llm_client_;
    ConfigManager &config_manager_;
    SessionManager &session_manager_;

    // Thread-safe cache for the last response of each model
    std::unordered_map<std::string, std::string> last_responses_;
    std::mutex mtx_;
};
} // namespace fusellm