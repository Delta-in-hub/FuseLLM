// src/fs/PathParser.h
#pragma once

#include <string>
#include <vector>
#include <optional>

namespace fusellm {

// 定义文件系统中的路径类型
enum class PathType {
    Root,
    ModelsDir,
    ModelFile,
    ConfigDir,
    GlobalSettingsFile,
    ModelSettingsFile,
    ConversationsDir,
    LatestConversationLink,
    ConversationDir,
    ConversationPrompt,
    ConversationHistory,
    ConversationContext,
    ConversationConfigDir,
    ConversationModelFile,
    ConversationSystemPrompt,
    ConversationSettingsFile,
    SemanticSearchDir,
    SearchIndexDir,
    SearchCorpusDir,
    SearchQueryFile,
    Unknown
};

// 封装解析后的路径信息
struct ParsedPath {
    PathType type = PathType::Unknown;
    std::string session_id;
    std::string model_name;
    std::string index_name;
    std::string filename;
};

// 路径解析工具类
class PathParser {
public:
    // 解析给定的路径字符串
    static ParsedPath parse(const std::string& path);
};

} // namespace fusellm
