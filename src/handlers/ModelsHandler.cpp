#include "ModelsHandler.h"
#include "../common/utils.hpp"
#include "../state/SessionManager.h"
#include <spdlog/spdlog.h>
#include <string.h>
#include <string_view>

namespace fusellm {

ModelsHandler::ModelsHandler(LLMClient &client, ConfigManager &config,
                             SessionManager &sessions)
    : llm_client_(client), config_manager_(config), session_manager_(sessions) {
    auto &default_model = config_manager_.default_model_;
    if (std::find(llm_client_.model_list.begin(), llm_client_.model_list.end(),
                  default_model) == llm_client_.model_list.end()) {
        SPDLOG_WARN("Default model '{}' not found in the model list.",
                    default_model);
        default_model = llm_client_.model_list.front();
        SPDLOG_WARN("Using default model '{}'.", default_model);
    }
}

int ModelsHandler::getattr(const char *path, struct stat *stbuf,
                           struct fuse_file_info *fi) {
    constexpr std::string_view models_dir = "models";
    constexpr std::string_view default_model = "default";

    // Initialize stat buffer
    memset(stbuf, 0, sizeof(struct stat));

    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');

    // Handle root path ("/")
    if (components.empty()) {
        return -ENOENT;
    }

    // Case 1: "/models" (directory)
    if (components.size() == 1 && components[0] == models_dir) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size = 4096;
        return 0;
    }

    // Case 2: "/models/<model_name>" (file)
    if (components.size() == 2 && components[0] == models_dir) {
        std::string_view model_name = components[1];
        bool is_valid_model =
            std::find(llm_client_.model_list.begin(),
                      llm_client_.model_list.end(),
                      model_name) != llm_client_.model_list.end() ||
            model_name == default_model;

        if (is_valid_model) {
            stbuf->st_mode = S_IFREG | 0666;
            stbuf->st_nlink = 1;
            stbuf->st_size = 4096;
            return 0;
        }
    }

    // All other cases: not found
    return -ENOENT;
}

int ModelsHandler::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi,
                           enum fuse_readdir_flags flags) {
    SPDLOG_DEBUG("Read directory '{}'", path);
    if (strcmp(path, "/models") != 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "default", NULL, 0, (fuse_fill_dir_flags)0);

    // In a real implementation, you would get this list from the config
    // For now, hardcoding some examples
    for (const auto &model : llm_client_.model_list) {
        filler(buf, model.c_str(), NULL, 0, (fuse_fill_dir_flags)0);
    }

    return 0;
}

int ModelsHandler::open(const char *path, struct fuse_file_info *fi) {
    SPDLOG_DEBUG("Open model '{}'", path);
    constexpr std::string_view models_dir = "models";
    constexpr std::string_view default_model = "default";
    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');
    if (components.size() == 1 && components[0] == models_dir) {
        return -EISDIR;
    }
    // Case 2: "/models/<model_name>" (file)
    if (components.size() == 2 && components[0] == models_dir) {
        std::string_view model_name = components[1];
        bool is_valid_model =
            std::find(llm_client_.model_list.begin(),
                      llm_client_.model_list.end(),
                      model_name) != llm_client_.model_list.end() ||
            model_name == default_model;

        if (is_valid_model) {
            return 0;
        }
    }
    SPDLOG_DEBUG("Invalid model name: {}", path);
    return -ENOENT;
}

int ModelsHandler::read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {

    SPDLOG_DEBUG("Read from model '{}'", path);
    constexpr std::string_view models_dir = "models";
    constexpr std::string_view default_model = "default";
    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');
    if (components.size() == 1 && components[0] == models_dir) {
        return -EISDIR;
    }
    // Case 2: "/models/<model_name>" (file)
    if (components.size() != 2 || components[0] != models_dir) {
        return -ENOENT;
    }
    std::string_view model_name = components[1];
    bool is_valid_model =
        std::find(llm_client_.model_list.begin(), llm_client_.model_list.end(),
                  model_name) != llm_client_.model_list.end() ||
        model_name == default_model;

    if (not is_valid_model) {
        return -ENOENT;
    }

    if (model_name == default_model) {
        model_name = config_manager_.default_model_;
    }

    SPDLOG_DEBUG("Read from model '{}'", model_name);

    std::string content;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = last_responses_.find(model_name.data());
        if (it != last_responses_.end()) {
            content = it->second;
        }
    }

    SPDLOG_DEBUG("Read from model '{}': {}", model_name, content);
    if (offset >= content.length()) {
        return 0; // Read past end of file
    }

    size_t len = std::min(size, content.length() - offset);
    memcpy(buf, content.data() + offset, len);
    return len;
}

int ModelsHandler::write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    constexpr std::string_view models_dir = "models";
    constexpr std::string_view default_model = "default";
    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');
    if (components.size() == 1 && components[0] == models_dir) {
        return -EISDIR;
    }
    // Case 2: "/models/<model_name>" (file)
    if (components.size() != 2 || components[0] != models_dir) {
        return -ENOENT;
    }
    std::string_view model_name = components[1];
    bool is_valid_model =
        std::find(llm_client_.model_list.begin(), llm_client_.model_list.end(),
                  model_name) != llm_client_.model_list.end() ||
        model_name == default_model;

    if (not is_valid_model) {
        return -ENOENT;
    }

    std::string prompt(buf, size);
    SPDLOG_INFO("Send query to model '{}': {}", model_name, prompt);

    if (model_name == default_model) {
        model_name = config_manager_.default_model_;
    }

    // 使用 ConfigManager 获取合并后的模型参数
    std::string response = llm_client_.simple_query(model_name, prompt, config_manager_);

    SPDLOG_DEBUG("Response from model '{}': {}", model_name, response);

    if (response.empty()) {
        SPDLOG_ERROR("LLM query failed for model '{}'", model_name);
        return -EIO; // Input/output error
    }

    // Create a new conversation to archive this stateless interaction.
    try {
        // Create a new session with an auto-generated, PID-like ID.
        // The creation logic, including retries, is now encapsulated in the
        // SessionManager.
        auto session = session_manager_.create_session_with_auto_id();

        if (session) {
            // Manually populate the new session with the prompt and response.
            session->populate(prompt, response);
            // This interaction also makes it the 'latest' session.
            session_manager_.set_latest_session_id(session->get_id());
            SPDLOG_INFO(
                "Archived stateless query as new conversation with ID: {}",
                session->get_id());
        } else {
            // This block should theoretically be unreachable now, but is kept
            // for robustness.
            SPDLOG_ERROR(
                "Failed to create a new session to archive the query.");
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("An exception occurred while creating archive session: {}",
                     e.what());
        // We don't return an error here, as the primary goal (getting a
        // response) succeeded. Archiving is a secondary concern.
    }

    {
        std::lock_guard<std::mutex> lock(mtx_);
        last_responses_[model_name.data()] = std::move(response);
    }

    return size; // On success, return the number of bytes written
}

} // namespace fusellm