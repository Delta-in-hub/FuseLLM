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
        stbuf->st_size = 4096; // Standard directory size
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
            stbuf->st_size = 4096; // Standard directory size
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
            // For regular files, set a reasonable default size that can be
            // changed later This helps tools like 'ls -l' and 'cat' work
            // properly
            stbuf->st_size = 1024; // Default size for settings.toml
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

            // 1. Get the actual model parameters from ConfigManager
            ModelParameters params =
                default_config.get_model_params(model_name);

            // 2. Serialize the parameters to TOML format
            std::stringstream ss;
            if (params.temperature) {
                ss << "temperature = " << *params.temperature << "\n";
            }
            if (params.system_prompt) {
                // Escape special characters in the string for TOML
                ss << "system_prompt = " << toml::value(*params.system_prompt)
                   << "\n";
            }

            std::string content = ss.str();

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

            // 不支持部分写入或追加，必须一次性写入整个文件
            if (offset != 0) {
                return -EPERM;
            }

            std::string content(buf, size);

            try {
                // 解析并验证 TOML 内容
                toml::table tbl = toml::parse(content);

                // 调用 ConfigManager 的更新方法
                if (!default_config.update_model_params(model_name, tbl)) {
                    SPDLOG_WARN(
                        "Validation failed for TOML content on model '{}'",
                        model_name);
                    return -EINVAL;
                }

                return size;
            } catch (const std::exception &e) {
                SPDLOG_WARN("Failed to parse TOML content for model '{}': {}",
                            model_name, e.what());
                return -EINVAL;
            }
        }
    }
    return -ENOENT;
}

} // namespace fusellm