#include "ConversationsHandler.h"
#include "../common/utils.hpp"
#include "../state/Session.h"
#include "src/config/ConfigManager.h"
#include <cerrno>
#include <spdlog/spdlog.h>
#include <string.h>
#include <string_view>
#include <vector>

namespace fusellm {

namespace { // Anonymous namespace for internal logic

// Defines the semantic type of a path WITHIN the /conversations directory.
enum class ConvPathType {
    Unknown,
    Root,        // /conversations
    LatestDir,   // /conversations/latest (acts as a directory)
    SessionDir,  // /conversations/<session_id>
    PromptFile,  // /conversations/<session_id>/prompt
    HistoryFile, // /conversations/<session_id>/history
    ContextFile, // /conversations/<session_id>/context
    ConfigDir,   // /conversations/<session_id>/config
    ModelFile,   // /conversations/<session_id>/config/model
    SettingsFile // /conversations/<session_id>/config/settings.toml
};

// A struct to hold the parsed path information.
struct ParsedConvPath {
    ConvPathType type = ConvPathType::Unknown;
    std::string session_id;
};

// Parses a path string (e.g., "/conversations/123/prompt") into a structured
// representation.
ParsedConvPath parse_conv_path(std::string_view path) {
    ParsedConvPath p;
    // Remove leading '/'
    if (strutil::starts_with(path, "/")) {
        path.remove_prefix(1);
    }

    std::vector<std::string> components = strutil::split(path, '/');

    // Path must start with "conversations"
    if (components.empty() || components[0] != "conversations") {
        return p;
    }

    if (components.size() == 1) { // "/conversations"
        p.type = ConvPathType::Root;
    } else if (components.size() == 2) { // "/conversations/<id>"
        p.session_id = components[1];
        p.type = (p.session_id == "latest") ? ConvPathType::LatestDir
                                            : ConvPathType::SessionDir;
    } else if (components.size() == 3) { // "/conversations/<id>/<file>"
        p.session_id = components[1];
        const auto &file = components[2];
        if (file == "prompt")
            p.type = ConvPathType::PromptFile;
        else if (file == "history")
            p.type = ConvPathType::HistoryFile;
        else if (file == "context")
            p.type = ConvPathType::ContextFile;
        else if (file == "config")
            p.type = ConvPathType::ConfigDir;
    } else if (components.size() == 4 &&
               components[2] ==
                   "config") { // "/conversations/<id>/config/<file>"
        p.session_id = components[1];
        const auto &file = components[3];
        if (file == "model")
            p.type = ConvPathType::ModelFile;
        else if (file == "settings.toml")
            p.type = ConvPathType::SettingsFile;
    }

    return p;
}

// Helper to get a session, resolving "latest" if necessary.
std::shared_ptr<Session> get_session(SessionManager &sm,
                                     const std::string &id) {
    if (id == "latest") {
        auto latest_id = sm.get_latest_session_id();
        if (latest_id.empty()) {
            return nullptr;
        }
        return sm.find_session(latest_id);
    }
    return sm.find_session(id);
}

} // namespace

ConversationsHandler::ConversationsHandler(SessionManager &sessions,
                                           LLMClient &client,
                                           ConfigManager &config)
    : session_manager_(sessions), llm_client_(client), config_manager_(config) {
}

int ConversationsHandler::getattr(const char *path_str, struct stat *stbuf,
                                  struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));
    ParsedConvPath p = parse_conv_path(path_str);

    switch (p.type) {
    case ConvPathType::Root:
    case ConvPathType::SessionDir:
    case ConvPathType::LatestDir:
    case ConvPathType::ConfigDir:
        if (p.type != ConvPathType::Root &&
            !get_session(session_manager_, p.session_id)) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size = 4096; // Standard directory size
        return 0;

    case ConvPathType::PromptFile:
    case ConvPathType::HistoryFile:
    case ConvPathType::ContextFile:
    case ConvPathType::ModelFile:
    case ConvPathType::SettingsFile: {
        if (!get_session(session_manager_, p.session_id)) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFREG | 0644; // rw-r--r--
        if (p.type == ConvPathType::HistoryFile) {
            stbuf->st_mode = S_IFREG | 0444; // Read-only
        }
        stbuf->st_nlink = 1;
        stbuf->st_size = 4096; // Report a non-zero size
        return 0;
    }

    default:
        return -ENOENT;
    }
}

int ConversationsHandler::readdir(const char *path, void *buf,
                                  fuse_fill_dir_t filler, off_t offset,
                                  struct fuse_file_info *fi,
                                  enum fuse_readdir_flags flags) {
    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);

    ParsedConvPath p = parse_conv_path(path);

    if (p.type == ConvPathType::Root) {
        // Only list 'latest' if a latest session ID actually exists
        if (!session_manager_.get_latest_session_id().empty()) {
            filler(buf, "latest", NULL, 0, (fuse_fill_dir_flags)0);
        }
        for (const auto &id : session_manager_.list_sessions()) {
            filler(buf, id.c_str(), NULL, 0, (fuse_fill_dir_flags)0);
        }
    } else if (p.type == ConvPathType::SessionDir ||
               p.type == ConvPathType::LatestDir) {
        if (!get_session(session_manager_, p.session_id))
            return -ENOENT;
        filler(buf, "prompt", NULL, 0, (fuse_fill_dir_flags)0);
        filler(buf, "history", NULL, 0, (fuse_fill_dir_flags)0);
        filler(buf, "context", NULL, 0, (fuse_fill_dir_flags)0);
        filler(buf, "config", NULL, 0, (fuse_fill_dir_flags)0);
    } else if (p.type == ConvPathType::ConfigDir) {
        if (!get_session(session_manager_, p.session_id))
            return -ENOENT;
        filler(buf, "model", NULL, 0, (fuse_fill_dir_flags)0);
        filler(buf, "settings.toml", NULL, 0, (fuse_fill_dir_flags)0);
    } else {
        return -ENOENT;
    }

    return 0;
}

int ConversationsHandler::mkdir(const char *path, mode_t mode) {
    ParsedConvPath p = parse_conv_path(path);
    if (p.type != ConvPathType::SessionDir) {
        return -EPERM;
    }

    if (session_manager_.create_session(p.session_id)) {
        SPDLOG_INFO("Created new conversation session: {}", p.session_id);
        return 0;
    }

    return -EEXIST;
}

int ConversationsHandler::rmdir(const char *path) {
    ParsedConvPath p = parse_conv_path(path);
    if (p.type != ConvPathType::SessionDir) {
        return -ENOTDIR;
    }

    if (session_manager_.remove_session(p.session_id)) {
        SPDLOG_INFO("Removed conversation session: {}", p.session_id);
        return 0;
    }
    return -ENOENT;
}

int ConversationsHandler::open(const char *path, struct fuse_file_info *fi) {
    ParsedConvPath p = parse_conv_path(path);
    if (p.type == ConvPathType::Unknown || p.type == ConvPathType::Root) {
        return -ENOENT;
    }

    // Check if underlying session exists for file operations
    if (p.type >= ConvPathType::PromptFile &&
        !get_session(session_manager_, p.session_id)) {
        return -ENOENT;
    }

    if (p.type == ConvPathType::HistoryFile &&
        (fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EACCES; // History is read-only
    }

    return 0;
}

int ConversationsHandler::read(const char *path, char *buf, size_t size,
                               off_t offset, struct fuse_file_info *fi) {
    ParsedConvPath p = parse_conv_path(path);
    auto session = get_session(session_manager_, p.session_id);
    if (!session) {
        return -ENOENT;
    }

    std::string content;
    switch (p.type) {
    case ConvPathType::PromptFile:
        content = session->get_latest_response();
        break;
    case ConvPathType::HistoryFile:
        content = session->get_formatted_history();
        break;
    case ConvPathType::ContextFile:
        content = session->get_context();
        break;
    case ConvPathType::ModelFile:
        content = session->get_model();
        break;
    case ConvPathType::SettingsFile:
        content = "";
        {
            ModelParameters params = session->get_settings();
            if (params.system_prompt) {
                content +=
                    "system_prompt = \"" + *params.system_prompt + "\"\n";
            }
            if (params.temperature) {
                content +=
                    "temperature = " + std::to_string(*params.temperature) +
                    "\n";
            }
        }
        break;
    default:
        return -EISDIR; // Cannot read a directory
    }

    if (offset >= content.length())
        return 0;
    size_t len = std::min(size, content.length() - offset);
    memcpy(buf, content.c_str() + offset, len);
    return len;
}

int ConversationsHandler::write(const char *path, const char *buf, size_t size,
                                off_t offset, struct fuse_file_info *fi) {
    // We assume that writes are atomic and overwrite the file's content.
    // This is typical for `echo "..." > file` shell commands.
    if (offset != 0) {
        // Appending (`>>`) is not supported for most files in this model.
        return -EPERM;
    }

    ParsedConvPath p = parse_conv_path(path);
    auto session = get_session(session_manager_, p.session_id);
    if (!session) {
        return -ENOENT;
    }

    // Update the 'latest' pointer to this session since it's being interacted
    // with.
    if (p.session_id != "latest") {
        session_manager_.set_latest_session_id(p.session_id);
    }

    std::string data(buf, size);

    switch (p.type) {
    case ConvPathType::PromptFile: {
        SPDLOG_INFO("Session '{}' received prompt.", session->get_id());
        std::string response = session->add_prompt(data, llm_client_);
        if (response.empty()) {
            return -EIO; // Input/Output Error on failed LLM call
        }
        break;
    }
    case ConvPathType::ContextFile:
        session->set_context(data);
        break;
    case ConvPathType::ModelFile:
        strutil::trim(data);
        session->set_model(data);
        break;
    case ConvPathType::SettingsFile: {
        ModelParameters params;
        try {
            auto v = toml::parse(data);
            bool flag = ModelParameters::validate_model_params_table(v);
            if (!flag) {
                return -EINVAL;
            }
            params.merge(v);
        } catch (const std::exception &e) {
            return -EINVAL;
        }
        session->set_settings(params);
    } break;
    default:
        return -EINVAL; // Invalid path for writing
    }

    return size;
}

} // namespace fusellm