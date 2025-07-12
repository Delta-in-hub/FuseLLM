#include "SemanticSearchHandler.h"
#include "../common/utils.hpp" // For strutil::split
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string.h>
#include <string_view>

namespace fusellm {

using json = nlohmann::json;

namespace { // Anonymous namespace for internal helpers

// Defines the semantic type of a path WITHIN the /semantic_search directory.
enum class SearchPathType {
    Unknown,
    Root,       // /semantic_search
    IndexDir,   // /semantic_search/<index_name>
    CorpusDir,  // /semantic_search/<index_name>/corpus
    CorpusFile, // /semantic_search/<index_name>/corpus/<doc.txt>
    QueryFile   // /semantic_search/<index_name>/query
};

// A struct to hold the parsed path information.
struct ParsedSearchPath {
    SearchPathType type = SearchPathType::Unknown;
    std::string index_name;
    std::string file_name;
};

// Parses a path string (e.g., "/semantic_search/my_idx/corpus/doc.txt")
// into a structured representation.
ParsedSearchPath parse_search_path(std::string_view path) {
    ParsedSearchPath p;
    // Remove leading '/'
    if (strutil::starts_with(path, "/")) {
        path.remove_prefix(1);
    }

    std::vector<std::string> components = strutil::split(path, '/');

    // Path must start with "semantic_search"
    if (components.empty() || components[0] != "semantic_search") {
        return p;
    }

    if (components.size() == 1) { // /semantic_search
        p.type = SearchPathType::Root;
    } else if (components.size() == 2) { // /semantic_search/<index_name>
        p.index_name = components[1];
        p.type = SearchPathType::IndexDir;
    } else if (components.size() ==
               3) { // /semantic_search/<index_name>/<dir_or_file>
        p.index_name = components[1];
        if (components[2] == "corpus") {
            p.type = SearchPathType::CorpusDir;
        } else if (components[2] == "query") {
            p.type = SearchPathType::QueryFile;
        }
    } else if (components.size() == 4 &&
               components[2] ==
                   "corpus") { // /semantic_search/<index_name>/corpus/<file>
        p.index_name = components[1];
        p.file_name = components[3];
        p.type = SearchPathType::CorpusFile;
    }

    return p;
}

// Helper to check if a ZMQ response indicates success.
bool is_response_ok(std::string_view response_str, std::string_view op_name) {
    if (response_str.empty()) {
        SPDLOG_ERROR("Received empty ZMQ response for operation '{}'",
                      op_name);
        return false;
    }
    json response = json::parse(response_str, nullptr, false);
    if (response.is_discarded()) {
        SPDLOG_ERROR("Failed to parse ZMQ JSON response for '{}': {}", op_name,
                      response_str);
        return false;
    }
    if (response.contains("error")) {
        SPDLOG_ERROR("ZMQ operation '{}' failed with error: {}", op_name,
                      response["error"].dump());
        return false;
    }
    return response.value("status", "") == "ok";
}

} // namespace

SemanticSearchHandler::SemanticSearchHandler(ZmqClient &client)
    : zmq_client_(client) {
    SPDLOG_DEBUG("SemanticSearchHandler initialized.");
}

std::vector<std::string> SemanticSearchHandler::list_indexes() {
    std::string response_str = zmq_client_.send_request("list_indexes", "{}");
    json response = json::parse(response_str, nullptr, false);
    if (response.is_discarded() || !response.is_array()) {
        SPDLOG_WARN("Could not list search indexes from backend. Response: {}",
                     response_str);
        return {};
    }
    return response.get<std::vector<std::string>>();
}

int SemanticSearchHandler::getattr(const char *path, struct stat *stbuf,
                                   struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));
    ParsedSearchPath p = parse_search_path(path);

    switch (p.type) {
    case SearchPathType::Root:
    case SearchPathType::CorpusDir:
        stbuf->st_mode = S_IFDIR | 0755; // rwxr-xr-x
        stbuf->st_nlink = 2;
        return 0;

    case SearchPathType::IndexDir: {
        // Check if the index actually exists
        auto indexes = list_indexes();
        if (std::find(indexes.begin(), indexes.end(), p.index_name) ==
            indexes.end()) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFDIR | 0755; // rwxr-xr-x
        stbuf->st_nlink = 2;
        return 0;
    }

    case SearchPathType::QueryFile:
        stbuf->st_mode = S_IFREG | 0644; // rw-r--r--
        stbuf->st_nlink = 1;
        // Size is dynamic, returning a non-zero placeholder is fine
        stbuf->st_size = 4096;
        return 0;

    case SearchPathType::CorpusFile:
        stbuf->st_mode = S_IFREG | 0644; // rw-r--r--
        stbuf->st_nlink = 1;
        // Corpus files are write-only in this model. Size is not tracked.
        stbuf->st_size = 4096;
        return 0;

    default:
        return -ENOENT;
    }
}

int SemanticSearchHandler::readdir(const char *path, void *buf,
                                   fuse_fill_dir_t filler, off_t offset,
                                   struct fuse_file_info *fi,
                                   enum fuse_readdir_flags flags) {
    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);

    ParsedSearchPath p = parse_search_path(path);

    if (p.type == SearchPathType::Root) {
        auto indexes = list_indexes();
        for (const auto &index_name : indexes) {
            filler(buf, index_name.c_str(), NULL, 0, (fuse_fill_dir_flags)0);
        }
    } else if (p.type == SearchPathType::IndexDir) {
        filler(buf, "corpus", NULL, 0, (fuse_fill_dir_flags)0);
        filler(buf, "query", NULL, 0, (fuse_fill_dir_flags)0);
    } else if (p.type == SearchPathType::CorpusDir) {
        json payload = {{"index_name", p.index_name}};
        std::string response_str =
            zmq_client_.send_request("list_documents", payload.dump());
        json response = json::parse(response_str, nullptr, false);
        if (response.is_array()) {
            for (const auto &doc_name : response) {
                filler(buf, doc_name.get<std::string>().c_str(), NULL, 0,
                       (fuse_fill_dir_flags)0);
            }
        }
    } else {
        return -ENOTDIR;
    }
    return 0;
}

int SemanticSearchHandler::mkdir(const char *path, mode_t mode) {
    ParsedSearchPath p = parse_search_path(path);
    if (p.type != SearchPathType::IndexDir) {
        SPDLOG_WARN("mkdir is only permitted for creating new indexes like "
                     "/semantic_search/<index_name>");
        return -EPERM; // Operation not permitted
    }

    json payload = {{"index_name", p.index_name}};
    std::string response_str =
        zmq_client_.send_request("create_index", payload.dump());

    if (!is_response_ok(response_str, "create_index")) {
        SPDLOG_ERROR("Failed to create search index '{}' via backend.",
                      p.index_name);
        return -EIO; // Input/output error
    }

    SPDLOG_INFO("Successfully created search index: {}", p.index_name);
    return 0;
}

int SemanticSearchHandler::rmdir(const char *path) {
    ParsedSearchPath p = parse_search_path(path);
    if (p.type != SearchPathType::IndexDir) {
        return -ENOTDIR;
    }

    json payload = {{"index_name", p.index_name}};
    std::string response_str =
        zmq_client_.send_request("delete_index", payload.dump());

    if (!is_response_ok(response_str, "delete_index")) {
        SPDLOG_ERROR("Failed to delete search index '{}' via backend.",
                      p.index_name);
        return -EIO;
    }

    SPDLOG_INFO("Successfully deleted search index: {}", p.index_name);
    return 0;
}

int SemanticSearchHandler::mknod(const char *path, mode_t mode, dev_t rdev) {
    ParsedSearchPath p = parse_search_path(path);
    if (p.type == SearchPathType::CorpusFile) {
        // mknod is called by commands like `touch`. We allow the creation
        // of an empty file in the corpus. The actual indexing happens on
        // write.
        return 0;
    }
    return -EPERM;
}

int SemanticSearchHandler::unlink(const char *path) {
    ParsedSearchPath p = parse_search_path(path);
    if (p.type != SearchPathType::CorpusFile) {
        return -EPERM;
    }

    SPDLOG_INFO("Removing document '{}' from index '{}'", p.file_name,
                 p.index_name);
    json payload = {{"index_name", p.index_name}, {"document_id", p.file_name}};
    std::string response_str =
        zmq_client_.send_request("remove_document", payload.dump());

    if (!is_response_ok(response_str, "remove_document")) {
        SPDLOG_ERROR("Failed to remove document '{}' from index '{}'",
                      p.file_name, p.index_name);
        return -EIO;
    }

    return 0;
}

int SemanticSearchHandler::open(const char *path, struct fuse_file_info *fi) {
    ParsedSearchPath p = parse_search_path(path);
    if (p.type == SearchPathType::Unknown) {
        return -ENOENT;
    }

    // According to the design, corpus files are write-only and query files are
    // r/w. We can enforce this here if needed, but for simplicity, we'll allow
    // opens. For example, reading a corpus file is blocked in the `read`
    // implementation.
    return 0;
}

int SemanticSearchHandler::read(const char *path, char *buf, size_t size,
                                off_t offset, struct fuse_file_info *fi) {
    ParsedSearchPath p = parse_search_path(path);
    if (p.type != SearchPathType::QueryFile) {
        // Reading corpus files is not a supported operation in this design.
        return -EACCES;
    }

    std::string content;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = last_query_results_.find(p.index_name);
        if (it != last_query_results_.end()) {
            content = it->second;
        } else {
            // If no query has been made yet, return a helpful message.
            content = "No query has been made for this index yet.\n";
        }
    }

    if (offset >= content.length())
        return 0; // Read past end of file

    size_t len = std::min(size, content.length() - offset);
    memcpy(buf, content.c_str() + offset, len);
    return len;
}

int SemanticSearchHandler::write(const char *path, const char *buf, size_t size,
                                 off_t offset, struct fuse_file_info *fi) {
    // This model assumes writes are not partial and overwrite the content.
    // The offset parameter is ignored for simplicity.
    ParsedSearchPath p = parse_search_path(path);

    if (p.type == SearchPathType::QueryFile) {
        std::string query_text(buf, size);
        strutil::trim(
            query_text); // Remove trailing newline often added by `echo`
        SPDLOG_INFO("Executing query on index '{}': {}", p.index_name,
                     query_text);

        json payload = {{"index_name", p.index_name}, {"query", query_text}};
        std::string response_str =
            zmq_client_.send_request("query", payload.dump());

        // We assume the backend returns a pre-formatted string as described
        // in Goal.md. If the backend returns an error, we display that instead.
        std::string final_content = response_str;

        {
            std::lock_guard<std::mutex> lock(mtx_);
            last_query_results_[p.index_name] = final_content;
        }
        return size;

    } else if (p.type == SearchPathType::CorpusFile) {
        std::string content(buf, size);
        SPDLOG_INFO("Indexing document '{}' ({} bytes) into index '{}'",
                     p.file_name, size, p.index_name);

        json payload = {{"index_name", p.index_name},
                        {"document_id", p.file_name},
                        {"text", content}};

        std::string response_str =
            zmq_client_.send_request("add_document", payload.dump());

        if (!is_response_ok(response_str, "add_document")) {
            SPDLOG_ERROR("Failed to index document '{}'", path);
            return -EIO;
        }

        return size;
    }

    return -EINVAL; // Invalid path for writing
}

} // namespace fusellm