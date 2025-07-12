#pragma once
#include "../services/ZmqClient.h"
#include "BaseHandler.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace fusellm {

/**
 * @class SemanticSearchHandler
 * @brief Manages all operations under the /semantic_search directory.
 *
 * This handler interfaces with the Python-based semantic search service via
 * ZeroMQ. It handles index creation/deletion and the core workflow of adding
 * documents to a corpus and executing queries.
 */
class SemanticSearchHandler : public BaseHandler {
  public:
    /**
     * @brief Constructs the handler.
     * @param client A reference to the ZmqClient for communicating with the
     * backend.
     */
    explicit SemanticSearchHandler(ZmqClient &client);

    // --- FUSE Overrides ---
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

    int unlink(const char *path) override;

    int mknod(const char *path, mode_t mode, dev_t rdev) override;

  private:
    ZmqClient &zmq_client_;

    // Thread-safe cache to store the last query result for each index.
    // The key is the index name (e.g., "my-codebase").
    std::map<std::string, std::string> last_query_results_;

    // Mutex to protect shared state like `last_query_results_`.
    mutable std::mutex mtx_;

    // Helper to get the list of active search indexes from the backend.
    std::vector<std::string> list_indexes();
};
} // namespace fusellm