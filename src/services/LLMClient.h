// src/services/LLMClient.h
#pragma once

#include "../common/data.h"
#include "../config/ConfigManager.h"
#include <string>
#include <vector>

namespace fusellm {

// 与 LLM API 通信的客户端接口
class LLMClient {
public:
    // 发起一次无状态的、一次性的请求
    std::string simple_query(const std::string& model, const std::string& prompt);

    // 发起一次有状态的、基于会话的请求
    std::string conversation_query(const Config& config, const Conversation& conversation);
};

} // namespace fusellm
