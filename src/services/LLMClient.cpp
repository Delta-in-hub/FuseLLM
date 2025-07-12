#include "LLMClient.h"
#include "external/openai-cpp/include/openai/openai.hpp"
#include "spdlog/spdlog.h"

namespace fusellm {

// Use nlohmann::json for convenience
using json = nlohmann::json;

LLMClient::LLMClient(const ConfigManager &config_manager) {
    const auto &api_key = config_manager.api_key_;
    const auto &base_url = config_manager.base_url_;

    if (api_key.empty()) {
        SPDLOG_WARN(
            "OpenAI API key is not set in the configuration. The client will "
            "rely on the OPENAI_API_KEY environment variable if present.");
    }

    try {
        // Initialize the openai-cpp library.
        // It supports an empty key, in which case it checks the environment
        // variable. If a base_url is provided, it configures it for custom
        // endpoints (e.g., local LLMs).
        if (base_url.empty()) {
            openai::start(api_key);
        } else {
            SPDLOG_INFO("Using custom LLM base URL: {}", base_url);
            // The library uses the third parameter for the base URL.
            openai::start(api_key, "", true, base_url);
        }

        // Fetch the list of available models
        auto models = openai::instance().model.list();
        for (const auto &model : models["data"]) {
            std::string model_id = model["id"].get<std::string>();
            model_list.push_back(model_id);
            SPDLOG_INFO("Available model: {}", model_id);
        }
    } catch (const std::exception &e) {
        // The library constructor might not throw, but this is good practice
        // for robustness.
        SPDLOG_ERROR("Failed to initialize LLM client: {}", e.what());
    }

    if (model_list.empty()) {
        SPDLOG_ERROR("No models found");
        throw std::runtime_error("No models found");
    }
}

std::string LLMClient::simple_query(std::string_view model_name,
                                    std::string_view prompt,
                                    const ModelParameters &ms) {
    // Construct a minimal message list for a simple, one-shot query.
    json messages;
    if (ms.system_prompt and not ms.system_prompt.value().empty()) {
        messages = json::array(
            {{{"role", "system"}, {"content", ms.system_prompt.value()}},
             {{"role", "user"}, {"content", prompt}}});
    } else {
        messages = json::array({{{"role", "user"}, {"content", prompt}}});
    }

    // Build the full JSON request body.
    json request_body = build_request_json(model_name, ms, messages);

    try {
        SPDLOG_DEBUG("Sending simple query to model '{}'", model_name);
        auto response = openai::chat().create(request_body);
        return extract_content_from_response(response);
    } catch (const std::exception &e) {
        SPDLOG_ERROR("LLM simple query failed for model '{}': {}", model_name,
                      e.what());
        return ""; // Return empty string on error to indicate failure.
    }
}

std::string LLMClient::conversation_query(std::string_view model_name,
                                          const ModelParameters &ms,
                                          const Conversation &conversation) {
    json messages = json::array();

    // 1. Add system prompt and context.
    // We combine the static system prompt from config and the dynamic context
    // into a single system message for the API for better context management.
    std::string final_system_prompt = ms.system_prompt.value_or("");
    if (!conversation.context.empty()) {
        if (!final_system_prompt.empty()) {
            final_system_prompt += "\n\n";
        }
        final_system_prompt += "ADDITIONAL CONTEXT FOR THIS CONVERSATION:\n" +
                               conversation.context;
    }

    if (!final_system_prompt.empty()) {
        messages.push_back(
            {{"role", "system"}, {"content", final_system_prompt}});
    }

    // 2. Add the entire conversation history.
    for (const auto &msg : conversation.history) {
        messages.push_back(
            {{"role", role_to_string(msg.role)}, {"content", msg.content}});
    }

    // Build the full JSON request body.
    json request_body = build_request_json(model_name, ms, messages);

    try {
        SPDLOG_DEBUG(
            "Sending conversation query to model '{}' with {} messages.",
            model_name, messages.size());
        auto response = openai::chat().create(request_body);
        return extract_content_from_response(response);
    } catch (const std::exception &e) {
        SPDLOG_ERROR("LLM conversation query failed for model '{}': {}",
                      model_name, e.what());
        return "";
    }
}

std::string LLMClient::role_to_string(Message::Role role) {
    switch (role) {
    case Message::Role::System:
        return "system";
    case Message::Role::User:
        return "user";
    case Message::Role::AI:
        return "assistant"; // The OpenAI API uses "assistant" for AI/model
                            // responses.
    }
    // This should ideally never be reached.
    return "user";
}

json LLMClient::build_request_json(std::string_view model_name,
                                   const ModelParameters &ms,
                                   const json &messages) {
    json request;
    request["model"] = model_name;
    request["messages"] = messages;

    // Add optional parameters to the request if they are set in the config.
    if (ms.temperature) {
        request["temperature"] = ms.temperature.value();
    }
    // Other parameters like max_tokens, top_p, etc., would be added here in the
    // same way. e.g., if (ms.max_tokens) { request["max_tokens"] =
    // *ms.max_tokens; }

    SPDLOG_INFO("Generated LLM request body: {}", request.dump(2));
    return request;
}

std::string
LLMClient::extract_content_from_response(const json &response_json) {
    SPDLOG_INFO("Received LLM response body: {}", response_json.dump(2));

    // Safely navigate the JSON structure to find the message content.
    if (response_json.contains("choices") &&
        response_json["choices"].is_array() &&
        !response_json["choices"].empty()) {
        const auto &first_choice = response_json["choices"][0];
        if (first_choice.contains("message") &&
            first_choice["message"].contains("content") &&
            first_choice["message"]["content"].is_string()) {
            return first_choice["message"]["content"].get<std::string>();
        }
    }

    // If we reach here, content was not found. Log the reason if possible.
    if (response_json.contains("error")) {
        SPDLOG_ERROR("LLM API returned an error: {}",
                      response_json["error"].dump());
    } else {
        SPDLOG_WARN(
            "Could not extract message content from LLM response. The "
            "'choices[0].message.content' path might be missing or invalid.");
    }

    return "";
}

} // namespace fusellm