#pragma once
#include "../config/ConfigManager.h"
#include "../services/LLMClient.h"
#include "../state/SessionManager.h"
#include "BaseHandler.h"

namespace fusellm {

/**
 * @class ConversationsHandler
 * @brief Manages all stateful interactions under the /conversations directory.
 *
 * This is the most complex handler, responsible for creating/deleting sessions
 * (directories), handling the core chat loop via the 'prompt' file, and
 * managing session-specific context and configuration.
 */
class ConversationsHandler : public BaseHandler {
  public:
    ConversationsHandler(SessionManager &sessions, LLMClient &client,
                         ConfigManager &config);

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
    int mkdir(const char *path, mode_t mode) override;
    int rmdir(const char *path) override;

  private:
    SessionManager &session_manager_;
    LLMClient &llm_client_;
    ConfigManager &config_manager_;
};
} // namespace fusellm