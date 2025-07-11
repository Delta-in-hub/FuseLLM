// src/state/Session.h
#pragma once

#include "../common/data.h"
#include "../config/ConfigManager.h"
#include <string>
#include <vector>
#include <mutex>

namespace fusellm {

// 管理单个会话的状态
class Session {
public:
    explicit Session(std::string id);

    // 获取会话ID
    std::string get_id() const;

    // 获取最新的AI回复
    std::string get_latest_response();

    // 添加用户提问并获取AI回复 (阻塞)
    // 返回AI的回答
    std::string add_prompt(const std::string& prompt);

    // 获取完整的对话历史
    std::string get_formatted_history();

    // 读写会话上下文
    void set_context(const std::string& context);
    std::string get_context();

    // 获取会话专属配置
    Config& get_config();

private:
    std::string id_;
    Conversation conversation_;
    Config session_config_; // 会话专属配置，覆盖上层配置
    std::string latest_response_; // 缓存最新的回复

    std::mutex mtx_; // 保护会话状态的读写
};

} // namespace fusellm
