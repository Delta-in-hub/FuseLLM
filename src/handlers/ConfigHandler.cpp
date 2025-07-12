#include "ConfigHandler.h"
#include "src/common/utils.hpp"
#include <spdlog/spdlog.h>
#include <string.h>

namespace fusellm {

ConfigHandler::ConfigHandler(ConfigManager &config, const LLMClient &client)
    : default_config(config), model_list_(client.model_list) {}

int ConfigHandler::getattr(const char *path, struct stat *stbuf,
                           struct fuse_file_info *fi) {
    constexpr std::string_view config_dir = "config";
    constexpr std::string_view default_model = "default";

    // Initialize stat buffer
    memset(stbuf, 0, sizeof(struct stat));

    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');

    // Handle root path ("/")
    if (components.empty()) {
        return -ENOENT;
    }

    // Case 1: "/config" (directory)
    if (components.size() == 1 && components[0] == config_dir) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    // Case 2: "/config/<model_name>/"
    if (components.size() == 2 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
    }

    // Case 3: "/config/<model_name>/settings.toml"
    if (components.size() == 3 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        std::string_view file_name = components[2];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model && file_name == "settings.toml") {
            stbuf->st_mode = S_IFREG | 0666;
            stbuf->st_nlink = 1;
            return 0;
        }
    }

    // All other cases: not found
    return -ENOENT;
}

int ConfigHandler::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi,
                           enum fuse_readdir_flags flags) {
    constexpr std::string_view config_dir = "config";
    constexpr std::string_view default_model = "default";

    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');

    // Case 1: "/config" (directory)
    if (components.size() == 1 && components[0] == config_dir) {
        filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
        filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);
        filler(buf, "default", NULL, 0, (fuse_fill_dir_flags)0);
        for (const auto &model : model_list_) {
            filler(buf, model.c_str(), NULL, 0, (fuse_fill_dir_flags)0);
        }
        return 0;
    }

    // Case 2: "/config/<model_name>"
    if (components.size() == 2 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model) {
            filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
            filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);
            filler(buf, "settings.toml", NULL, 0, (fuse_fill_dir_flags)0);
            return 0;
        }
    }

    return -ENOENT;
}

int ConfigHandler::open(const char *path, struct fuse_file_info *fi) {
    constexpr std::string_view config_dir = "config";
    constexpr std::string_view default_model = "default";

    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');

    // Case 1: "/config" (directory)
    if (components.size() == 1 && components[0] == config_dir) {
        return -EISDIR;
    }

    // Case 2: "/config/<model_name>"
    if (components.size() == 2 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model) {
            return -EISDIR;
        }
    }

    // Case 3: "/config/<model_name>/settings.toml"
    if (components.size() == 3 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        std::string_view file_name = components[2];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model && file_name == "settings.toml") {
            return 0;
        }
    }

    return -ENOENT;
}

int ConfigHandler::read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    constexpr std::string_view config_dir = "config";
    constexpr std::string_view default_model = "default";

    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');

    // Case 1: "/config" (directory)
    if (components.size() == 1 && components[0] == config_dir) {
        return -EISDIR;
    }

    // Case 2: "/config/<model_name>"
    if (components.size() == 2 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model) {
            return -EISDIR;
        }
    }

    // Case 3: "/config/<model_name>/settings.toml"
    if (components.size() == 3 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        std::string_view file_name = components[2];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model && file_name == "settings.toml") {
            if (model_name == default_model) {
                model_name = default_config.default_model_;
            }
            std::string content;
            {
                std::lock_guard<std::mutex> lock(mtx_);
                auto it = models_config.find(model_name.data());
                if (it != models_config.end()) {
                    content = it->second;
                }
            }

            if (offset >= content.length()) {
                return 0;
            }
            size_t len = std::min(size, content.length() - offset);
            memcpy(buf, content.c_str() + offset, len);
            return len;
        }
    }
    return -ENOENT;
}

int ConfigHandler::write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    constexpr std::string_view config_dir = "config";
    constexpr std::string_view default_model = "default";
    // Split path into components (skip leading '/')
    const auto components = strutil::split(path + 1, '/');

    // Case 1: "/config" (directory)
    if (components.size() == 1 && components[0] == config_dir) {
        return -EISDIR;
    }

    // Case 2: "/config/<model_name>"
    if (components.size() == 2 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model) {
            return -EISDIR;
        }
    }

    // Case 3: "/config/<model_name>/settings.toml"
    if (components.size() == 3 && components[0] == config_dir) {
        std::string_view model_name = components[1];
        std::string_view file_name = components[2];
        bool is_valid_model = std::find(model_list_.begin(), model_list_.end(),
                                        model_name) != model_list_.end() ||
                              model_name == default_model;

        if (is_valid_model && file_name == "settings.toml") {
            if (model_name == default_model) {
                model_name = default_config.default_model_;
            }
            std::string content;
            {
                std::lock_guard<std::mutex> lock(mtx_);
                auto it = models_config.find(model_name.data());
                if (it != models_config.end()) {
                    content = it->second;
                }
            }
            // offset
            if (offset + size >= content.length()) {
                content.resize(offset + size);
            }
            content.replace(offset, size, buf);

            // validate toml
            bool valid = false;
            try {
                valid = ModelParameters::validate_model_params_table(
                    toml::parse(content));
            } catch (const std::exception &e) {
                SPDLOG_WARN("Validation failed: {} : {}", e.what(), content);
                return -EINVAL;
            }
            if (!valid) {
                return -EINVAL;
            }
            {
                std::lock_guard<std::mutex> lock(mtx_);
                models_config[model_name.data()] = content;
            }
            return size;
        }
    }
    return -ENOENT;
}

} // namespace fusellm