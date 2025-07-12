#pragma once

#include <optional>
#include <string>
#include <vector>

namespace fusellm {

// Defines the semantic type of a path within the FuseLLM filesystem.
enum class PathType {
    // Top-level
    Root,

    // /models path types
    Models, // e.g., /models/gpt-4

    // /config path types
    Config,
    // /config/settings.toml
    // /config/models
    // /config/models/gpt-4/settings.toml

    // /conversations path types
    Conversations,
    // /conversations/latest
    // /conversations/session_123
    // /conversations/session_123/prompt
    // /conversations/session_123/history
    // /conversations/session_123/context
    // /conversations/session_123/config
    // /conversations/session_123/config/model
    // /conversations/session_123/config/system_prompt
    // /conversations/session_123/config/settings.toml

    // /semantic_search path types
    SemanticSearch,
    // /semantic_search/my_index
    // /semantic_search/my_index/corpus
    // /semantic_search/my_index/corpus/doc.txt
    // /semantic_search/my_index/query

    // Fallback
    Other
};

// A utility class responsible for parsing a string path into a PathType
class PathParser {
  public:
    /**
     * @brief Parses a given path string and returns its structured
     * representation.
     * @param path The absolute path string within the FUSE filesystem (e.g.,
     * "/conversations/123/prompt").
     * @return A PathType enum value representing the type of the path.
     */
    static PathType parse(std::string_view path);
};

} // namespace fusellm