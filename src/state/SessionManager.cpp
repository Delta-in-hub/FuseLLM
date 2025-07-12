#include "SessionManager.h"

namespace fusellm {

SessionManager::SessionManager(const ConfigManager &config)
    : config_manager_(config) {}

std::shared_ptr<Session> SessionManager::create_session(std::string_view id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (sessions_.count(id.data())) {
        return nullptr; // Session with this ID already exists
    }

    auto session = std::make_shared<Session>(id.data(), config_manager_);
    sessions_[id.data()] = session;
    return session;
}

bool SessionManager::remove_session(std::string_view id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (latest_session_id_ == id) {
        latest_session_id_.clear();
    }
    return sessions_.erase(id.data()) > 0;
}

std::shared_ptr<Session> SessionManager::find_session(std::string_view id) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = sessions_.find(id.data());
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> SessionManager::list_sessions() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::string> ids;
    ids.reserve(sessions_.size());
    for (const auto &pair : sessions_) {
        ids.push_back(pair.first);
    }
    return ids;
}

void SessionManager::set_latest_session_id(std::string_view id) {
    std::lock_guard<std::mutex> lock(mtx_);
    latest_session_id_ = id;
}

std::string SessionManager::get_latest_session_id() {
    std::lock_guard<std::mutex> lock(mtx_);
    return latest_session_id_;
}

} // namespace fusellm