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
            SPDLOG_WARN(
                "Validation failed: 'system_prompt' must be a string.");
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

} // namespace fusellm