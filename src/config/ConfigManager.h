#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>
#include <unordered_map>

namespace fusellm {

/**
 * @struct ModelParameters
 * @brief Represents a set of configurable parameters for an LLM.
 * This structure can be layered (global, model-specific, session-specific).
 */
struct ModelParameters {
    /**
     * @brief Merges parameters from a TOML table into this object.
     * @param tbl The TOML table to load settings from.
     */
    void merge(const toml::table &tbl);

    /**
     * @brief Validates the structure and values of a TOML table containing
     * model parameters.
     * @param tbl The TOML table to validate.
     * @return True if the table is valid, false otherwise.
     */
    static bool validate_model_params_table(const toml::table &tbl);

    std::optional<double> temperature;
    std::optional<std::string> system_prompt;
    // Other potential LLM parameters like top_p, max_tokens can be added here.
};

/**
 * @class ConfigManager
 * @brief Manages the overall application and model configurations.
 * This class is thread-safe.
 */
class ConfigManager {
  public:
    ConfigManager();

    /**
     * @brief Loads the global configuration from a TOML file.
     * @param path The path to the configuration file.
     * @return True on success, false on failure (e.g., file not found, parse
     * error).
     */
    bool load_from_file(std::string_view path);

    // Top-level settings from the global config file.
    std::string default_model_;
    std::string api_key_;
    std::string base_url_;
    std::string semantic_search_service_url_;

    // Parsed configuration objects.
    ModelParameters global_params_;
};

} // namespace fusellm