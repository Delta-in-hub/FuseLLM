#include "ConfigManager.h"
#include "spdlog/spdlog.h"
#include "src/common/utils.hpp"

namespace fusellm {

// --- ModelParameters Implementation ---

void ModelParameters::merge(const toml::table &tbl) {
    if (auto temp_node = tbl["temperature"];
        temp_node && temp_node.is_number()) {
        temperature = temp_node.value<double>();
    }
    if (auto prompt_node = tbl["system_prompt"];
        prompt_node && prompt_node.is_string()) {
        system_prompt = prompt_node.value<std::string>();
    }
    // Add merging for other parameters here.
}

void ModelParameters::merge(const ModelParameters &other) {
    // Only override if the other parameter has a value
    if (other.temperature) {
        temperature = other.temperature;
    }
    if (other.system_prompt) {
        system_prompt = other.system_prompt;
    }
    // Add merging for other parameters here as they are added
}

// --- ConfigManager Implementation ---

ConfigManager::ConfigManager()
    // Initialize with hardcoded defaults, which will be overridden by the
    // config file.
    : default_model_("deepseek-v3"),
      semantic_search_service_url_("ipc:///tmp/fusellm-semantic.ipc") {
    // The global_params_ starts with all its std::optional members as
    // std::nullopt.
}

bool ConfigManager::load_from_file(std::string_view path) {

    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error &err) {
        SPDLOG_ERROR("Failed to parse config file '{}':\n{}", path,
                     err.description());
        return false;
    }

    // Load top-level settings
    default_model_ = tbl["default_model"].value_or("deepseek-v3");
    api_key_ = tbl["api_key"].value_or("");
    base_url_ = tbl["base_url"].value_or("");
    if (not strutil::ends_with(base_url_, "/")) {
        base_url_ += "/";
    }

    // Load semantic search settings from its own table
    if (auto *search_tbl = tbl["semantic_search"].as_table()) {
        semantic_search_service_url_ =
            search_tbl->get("service_url")
                ->value_or("ipc:///tmp/fusellm-semantic.ipc");
    }

    // Load global default parameters from the [default_config] table
    if (auto *default_config_tbl = tbl["default_config"].as_table()) {
        if (ModelParameters::validate_model_params_table(*default_config_tbl)) {
            global_params_.merge(*default_config_tbl);
            if (not global_params_.system_prompt.has_value()) {
                global_params_.system_prompt =
                    "FuseLLM\nMount your LLM.\nEverything is a file.Even the "
                    "LLM.";
            }
        } else {
            SPDLOG_WARN(
                "Invalid values found in [default_config] section of '{}'.",
                path);
        }
    }

    SPDLOG_INFO("Successfully loaded configuration from '{}'.", path);
    return true;
}

bool ModelParameters::validate_model_params_table(const toml::table &tbl) {
    if (auto temp_node = tbl.get("temperature")) {
        if (!temp_node->is_number()) {
            SPDLOG_WARN("Validation failed: 'temperature' must be a number.");
            return false;
        }
        auto temp = temp_node->value<double>();
        if (not temp.has_value() || temp < 0.0 || temp > 2.0) {
            SPDLOG_WARN("Validation failed: 'temperature' must be between 0.0 "
                        "and 2.0.");
            return false;
        }
    }

    if (auto prompt_node = tbl.get("system_prompt")) {
        if (!prompt_node->is_string()) {
            SPDLOG_WARN("Validation failed: 'system_prompt' must be a string.");
            return false;
        }
    }

    for (const auto &[key, _] : tbl) {
        const auto key_str = std::string(key.str());
        if (key_str != "temperature" && key_str != "system_prompt") {
            SPDLOG_WARN(
                "Validation warning: Unknown configuration key '{}' found.",
                key_str);
        }
    }

    return true;
}

bool ConfigManager::update_model_params(std::string_view model_name,
                                        const toml::table &tbl) {
    if (!ModelParameters::validate_model_params_table(tbl)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mtx_);
    // 获取或创建该模型的参数对象，然后合并新设置
    model_specific_params_[std::string(model_name)].merge(tbl);

    SPDLOG_INFO("Updated configuration for model '{}'.", model_name);
    return true;
}

ModelParameters
ConfigManager::get_model_params(std::string_view model_name) const {
    // 从全局参数开始
    ModelParameters final_params = global_params_;

    std::lock_guard<std::mutex> lock(mtx_);
    auto it = model_specific_params_.find(std::string(model_name));
    if (it != model_specific_params_.end()) {
        // 如果有模型专属配置，合并它们（专属配置覆盖全局配置）
        final_params.merge(it->second);
    }
    return final_params;
}

} // namespace fusellm