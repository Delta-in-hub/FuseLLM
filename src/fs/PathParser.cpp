#include "PathParser.h"
#include "../common/utils.hpp" // For strutil::split
#include <spdlog/spdlog.h>

namespace fusellm {

PathType PathParser::parse(std::string_view path) {
    // SPDLOG_DEBUG("Parsing path: {}", path);
    if (path == "/") {
        return PathType::Root;
    }

    // Split the path, removing the initial '/'
    std::vector<std::string> components = strutil::split(path.substr(1), '/');

    if (components.empty()) {
        return PathType::Other;
    }

    const auto &root_dir = components[0];

    if (root_dir == "models") {
        return PathType::Models;
    } else if (root_dir == "config") {
        return PathType::Config;
    } else if (root_dir == "conversations") {
        return PathType::Conversations;
    } else if (root_dir == "semantic_search") {
        return PathType::SemanticSearch;
    }
    return PathType::Other;
}

} // namespace fusellm