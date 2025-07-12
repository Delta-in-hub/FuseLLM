#include "Session.h"
#include "spdlog/spdlog.h"

namespace fusellm {

Session::Session(std::string_view id, const ConfigManager &global_config)
    : id_(id) {
    // Initialize the session with default settings from the global config
    model_name_ = global_config.default_model_;
    session_params_ = global_config.global_params_;
}

std::string Session::get_id() const { return id_; }

std::string Session::get_latest_response() {
    std::lock_guard<std::mutex> lock(mtx_);
    return latest_response_;
}

std::string Session::get_context() {
    std::lock_guard<std::mutex> lock(mtx_);
    return conversation_.context;
}

void Session::set_context(std::string_view context) {
    std::lock_guard<std::mutex> lock(mtx_);
    // Overwrite the previous context
    conversation_.context = context;
    SPDLOG_DEBUG("Context set for session '{}'", id_);
}

std::string Session::get_model() {
    std::lock_guard<std::mutex> lock(mtx_);
    return model_name_;
}

void Session::set_model(std::string_view model_name) {
    std::lock_guard<std::mutex> lock(mtx_);
    model_name_ = model_name;
    SPDLOG_DEBUG("Model for session '{}' set to '{}'", id_, model_name_);
}

ModelParameters Session::get_settings() {
    std::lock_guard<std::mutex> lock(mtx_);
    return session_params_;
}

void Session::set_settings(ModelParameters params) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (params.system_prompt) {
        session_params_.system_prompt = params.system_prompt;
    }
    if (params.temperature) {
        session_params_.temperature = params.temperature;
    }
    SPDLOG_DEBUG("Settings for session '{}' updated", id_);
}

std::string Session::get_formatted_history() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::stringstream ss;

    if (session_params_.system_prompt) {
        ss << "[SYSTEM]\n" << *session_params_.system_prompt << "\n\n";
    }

    for (const auto &msg : conversation_.history) {
        switch (msg.role) {
        case Message::Role::User:
            ss << "[USER]\n" << msg.content << "\n\n";
            break;
        case Message::Role::AI:
            ss << "[AI]\n" << msg.content << "\n\n";
            break;
        default:
            break; // Ignore system messages in this view
        }
    }
    return ss.str();
}

std::string Session::add_prompt(std::string_view prompt,
                                LLMClient &llm_client) {
    std::lock_guard<std::mutex> lock(mtx_);

    // 1. Add user message to history
    conversation_.history.push_back(Message{Message::Role::User,
                                            std::string(prompt),
                                            std::chrono::system_clock::now()});
    SPDLOG_INFO("Session '{}': Added user prompt.", id_);

    // 2. Call the LLM
    // The conversation_query method will handle the context and history
    std::string response = llm_client.conversation_query(
        model_name_, session_params_, conversation_);

    if (response.empty()) {
        SPDLOG_ERROR("Session '{}': Received empty response from LLMClient.",
                      id_);
        // Revert the history on failure
        conversation_.history.pop_back();
        return ""; // Indicate failure
    }

    // 3. Store the AI's response
    latest_response_ = response;
    conversation_.history.push_back(
        {Message::Role::AI, response, std::chrono::system_clock::now()});
    SPDLOG_INFO("Session '{}': Stored AI response.", id_);

    return response;
}

} // namespace fusellm
