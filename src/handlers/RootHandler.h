#pragma once
#include "BaseHandler.h"

namespace fusellm {

/**
 * @class RootHandler
 * @brief Handles operations for the root ("/") directory.
 *
 * Its primary responsibility is to list the top-level directories:
 * 'models', 'config', 'conversations', and 'semantic_search'.
 */
class RootHandler : public BaseHandler {
  public:
    int getattr(const char *path, struct stat *stbuf,
                struct fuse_file_info *fi) override;

    int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags) override;
};
} // namespace fusellm