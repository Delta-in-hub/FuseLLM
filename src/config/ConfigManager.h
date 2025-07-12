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
     * @brief Merges parameters from another ModelParameters object into this one.
     * Only non-nullopt values from 'other' will override current values.
     * @param other The other ModelParameters to merge from.
     */
    void merge(const ModelParameters &other);

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

    /**
     * @brief 更新特定模型的配置参数。
     * @param model_name 要更新配置的模型名称。
     * @param tbl 从文件写入中解析出的 TOML 表。
     * @return 如果更新成功（且验证通过），则返回 true。
     */
    bool update_model_params(std::string_view model_name, const toml::table& tbl);

    /**
     * @brief 获取指定模型最终生效的参数。
     * 该方法会合并全局参数和模型专属参数。
     * @param model_name 模型名称。
     * @return 合并后的 ModelParameters 对象。
     */
    ModelParameters get_model_params(std::string_view model_name) const;

  protected:
    std::unordered_map<std::string, ModelParameters> model_specific_params_;
    mutable std::mutex mtx_; // 用于保护 model_specific_params_
};

} // namespace fusellm