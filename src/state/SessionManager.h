// src/state/SessionManager.h
#pragma once

#include "Session.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace fusellm {

// 管理所有活动会话的生命周期
class SessionManager {
public:
    // 创建一个新会话，如果id已存在则失败
    // 返回指向新会a话的共享指针，若失败则返回nullptr
    std::shared_ptr<Session> create_session(const std::string& id);

    // 删除一个会话
    // 返回 true 如果成功删除
    bool remove_session(const std::string& id);

    // 查找一个会话
    // 返回指向会话的共享指针，若未找到则返回nullptr
    std::shared_ptr<Session> find_session(const std::string& id);

    // 列出所有会话的ID
    std::vector<std::string> list_sessions();

    // 更新并获取最新的会话ID
    void set_latest_session_id(const std::string& id);
    std::string get_latest_session_id();

private:
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    std::string latest_session_id_;
    std::mutex mtx_; // 保护 sessions_ 映射和 latest_session_id_
};

} // namespace fusellm
