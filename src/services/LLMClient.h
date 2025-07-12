#pragma once

#include "../common/data.h"
#include "../config/ConfigManager.h"
#include "nlohmann/json.hpp"
#include <string>

namespace fusellm {

/**
 * @class LLMClient
 * @brief A client for interacting with a Large Language Model API (like
 * OpenAI).
 *
 * This class encapsulates the logic for sending requests to an LLM,
 * handling both stateless single queries and stateful, multi-turn
 * conversations. It uses the application's configuration to manage API keys and
 * endpoints.
 */
class LLMClient {
  public:
    /**
     * @brief Constructs an LLMClient.
     * @param config_manager A reference to the application's configuration
     * manager.
     *
     * The constructor initializes the underlying OpenAI C++ library with the
     * API key and base URL provided by the ConfigManager.
     */
    explicit LLMClient(const ConfigManager &config_manager);

    std::vector<std::string> model_list;

    /**
     * @brief Sends a simple, stateless query to the LLM.
     * @param model_name The name of the model to use (e.g., "gpt-4").
     * @param prompt The user's question or prompt.
     * @param config_manager Reference to the ConfigManager to get model parameters.
     * @return The LLM's response as a string, or an empty string on failure.
     */
    std::string simple_query(std::string_view model_name,
                             std::string_view prompt,
                             const ConfigManager &config_manager);

    /**
     * @brief Sends a request based on a full conversation history.
     * @param model_name The name of the model to use.
     * @param params The fully resolved model parameters, including any system
     * prompt.
     * @param conversation The conversation object, containing history and
     * context. The last message in the history is assumed to be the user's
     * latest prompt.
     * @return The LLM's response as a string, or an empty string on failure.
     */
    std::string conversation_query(std::string_view model_name,
                                   const ConfigManager &config_manager,
                                   const Conversation &conversation);

  private:
    /**
     * @brief Converts the internal Message::Role enum to its string
     * representation for the OpenAI API.
     * @param role The enum role.
     * @return A string ("system", "user", or "assistant").
     */
    static std::string role_to_string(Message::Role role);

    /**
     * @brief Constructs the JSON request body for the LLM API call.
     * @param model_name The model identifier.
     * @param params The model parameters.
     * @param messages The list of messages for the conversation.
     * @return A nlohmann::json object ready to be sent to the API.
     */
    static nlohmann::json build_request_json(std::string_view model_name,
                                             const ModelParameters &ms,
                                             const nlohmann::json &messages);

    /**
     * @brief Extracts the response content from the API's JSON reply.
     * @param response_json The JSON object returned by the API.
     * @return The content of the first choice message, or an empty string.
     */
    static std::string
    extract_content_from_response(const nlohmann::json &response_json);
};

} // namespace fusellm