// src/config/ConfigManager.h
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <toml++/toml.hpp>

namespace fusellm {

// 简化配置访问的结构体
struct Config {
    // 从TOML表中加载或合并配置
    void load(const toml::table& tbl);

    std::string model = "default";
    double temperature = 0.7;
    std::string system_prompt = "You are a helpful assistant.";
    // ... 其他可配置项
};

// 负责加载、验证和提供对所有配置层级的访问
class ConfigManager {
public:
    ConfigManager();

    // 从文件加载全局配置
    bool load_global_config(const std::string& path);

    // 获取合并后的最终配置，考虑所有层级
    // (会话 > 模型 > 全局)
    Config get_effective_config(const std::string& session_id, const std::string& model_name);

    // 读写全局/模型配置文件内容
    std::string read_config_file(const std::string& path);
    bool write_config_file(const std::string& path, const std::string& content);

private:
    // 验证TOML内容的合法性
    bool validate_config(const toml::table& tbl);

    Config global_config_;
    std::unordered_map<std::string, Config> model_configs_;
    // 会话配置由 Session 对象自己管理

    std::mutex mtx_; // 保护配置的读写
};

} // namespace fusellm
