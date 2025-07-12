#pragma once

#include "../common/data.h"
#include "../config/ConfigManager.h"
#include "../services/LLMClient.h"
#include <mutex>
#include <string>
#include <string_view>

namespace fusellm {

/**
 * @class Session
 * @brief Manages the state of a single, stateful conversation.
 *
 * This class encapsulates the entire history, context, and configuration for a
 * chat session. It is responsible for interacting with the LLMClient to get new
 * responses. This class is thread-safe.
 */
class Session {
  public:
    /**
     * @brief Constructs a new Session with a unique identifier.
     * @param id The unique string identifier for this session.
     * @param global_config A reference to the application's ConfigManager to
     * inherit base model parameters.
     */
    explicit Session(std::string_view id, const ConfigManager &global_config);

    // Getters for session properties
    std::string get_id() const;
    std::string get_latest_response();
    std::string get_formatted_history();
    std::string get_context();
    std::string get_model();
    ModelParameters get_settings();

    // Setters for session properties
    void set_context(std::string_view context);
    void set_model(std::string_view model_name);
    void set_settings(ModelParameters params);

    /**
     * @brief The core interactive function.
     *
     * Takes a user prompt, adds it to the history, sends the entire
     * conversation to the LLM via the client, and stores the response.
     *
     * @param prompt The user's new message.
     * @param llm_client The client to use for the API call.
     * @return The AI's response as a string.
     */
    std::string add_prompt(std::string_view prompt, LLMClient &llm_client);

    /**
     * @brief Manually populates the session with a user prompt and an AI response.
     *
     * This is used for creating a session record from a stateless interaction,
     * such as one initiated from the /models directory, without triggering a
     * new LLM call.
     *
     * @param user_prompt The initial user message.
     * @param ai_response The corresponding AI response.
     */
    void populate(std::string_view user_prompt, std::string_view ai_response);

  private:
    std::string id_;
    Conversation conversation_;
    std::string latest_response_;

    // Session-specific configuration overrides
    ModelParameters session_params_;
    std::string model_name_;

    // A mutex to protect all read/write operations on the session's state
    std::mutex mtx_;
};

} // namespace fusellm