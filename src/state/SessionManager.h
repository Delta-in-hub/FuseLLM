#pragma once

#include "Session.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace fusellm {

/**
 * @class SessionManager
 * @brief A thread-safe manager for the lifecycle of all chat sessions.
 *
 * This class creates, finds, deletes, and lists all active Session objects.
 * It also tracks the most recently used session to support the 'latest'
 * symlink.
 */
class SessionManager {
  public:
    /**
     * @brief Constructs the SessionManager.
     * @param config A reference to the global ConfigManager, needed to
     * initialize new sessions with default settings.
     */
    explicit SessionManager(const ConfigManager &config);

    /**
     * @brief Creates a new session with the given ID.
     * @param id The unique identifier for the new session.
     * @return A shared pointer to the new Session, or nullptr if a session
     * with that ID already exists.
     */
    std::shared_ptr<Session> create_session(std::string_view id);

    /**
     * @brief Removes a session by its ID.
     * @param id The ID of the session to remove.
     * @return True if the session was found and removed, false otherwise.
     */
    bool remove_session(std::string_view id);

    /**
     * @brief Finds a session by its ID.
     * @param id The ID of the session to find.
     * @return A shared pointer to the Session, or nullptr if not found.
     */
    std::shared_ptr<Session> find_session(std::string_view id);

    /**
     * @brief Lists the IDs of all currently active sessions.
     * @return A vector of strings containing the session IDs.
     */
    std::vector<std::string> list_sessions();

    /**
     * @brief Updates the ID of the most recently interacted-with session.
     * @param id The ID of the latest session.
     */
    void set_latest_session_id(std::string_view id);

    /**
     * @brief Retrieves the ID of the most recently interacted-with session.
     * @return The string ID of the latest session.
     */
    std::string get_latest_session_id();

  private:
    // A reference to the global config manager to pass to new sessions
    const ConfigManager &config_manager_;

    // The primary storage for sessions, mapping ID to a session object
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    std::string latest_session_id_;

    // A single mutex to protect both the map and the 'latest_session_id_'
    // string from concurrent access.
    std::mutex mtx_;
};

} // namespace fusellm