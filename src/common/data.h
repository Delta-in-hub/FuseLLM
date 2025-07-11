// src/common/data.h
#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace fusellm {

// 代表一次 LLM 交互中的单条消息
struct Message {
    enum class Role {
        System,
        User,
        AI
    };

    Role role;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
};

// 代表一次完整的会话
struct Conversation {
    std::string id;
    std::vector<Message> history; // 问答历史
    std::string context;          // 临时上下文
    // 会话特定的配置将通过 ConfigManager 获取
};

// 代表语义搜索的结果
struct SearchResult {
    float score;
    std::string source_file; // 来源语料文件
    std::string content;     // 相关的文本片段
};

} // namespace fusellm
