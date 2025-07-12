// src/main.cpp
#include "cxxopts.hpp"
#include "fs/FuseLLM.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <iostream>

int main(int argc, char *argv[]) {
    // 1. 设置日志
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] [%!] %v");

    SPDLOG_INFO("Starting FuseLLM...");

    // 2. 解析命令行参数 (如 mountpoint, config file)
    cxxopts::Options options("FuseLLM", "Mount your LLM as a filesystem.");

    options.add_options()("m,mountpoint", "Mount point for the filesystem",
                          cxxopts::value<std::string>())(
        "c,config", "Path to global configuration file",
        cxxopts::value<std::string>())("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (not result.count("mountpoint")) {
        SPDLOG_ERROR("Mount point is required.");
        std::cout << options.help() << std::endl;
        return 1;
    }
    std::string mountpoint = result["mountpoint"].as<std::string>();

    fusellm::ConfigManager global_config;

    if (not result.count("config")) {
        SPDLOG_WARN("No config file specified, using default.");
    } else {
        std::string config_path = result["config"].as<std::string>();

        // check file exists
        if (not std::filesystem::exists(config_path)) {
            SPDLOG_ERROR("Config file does not exist: '{}'.", config_path);
            return 1;
        }

        if (not global_config.load_from_file(config_path)) {
            SPDLOG_ERROR("Failed to load config file '{}'.", config_path);
            return 1;
        }
    }

    // 3. 初始化 FuseLLM 实例
    fusellm::FuseLLM fs(global_config);

    // 4. 准备 FUSE 参数
    // fusepp 会处理大部分参数，但我们需要把我们的 mountpoint 加进去
    std::vector<char *> fuse_args;
    fuse_args.push_back(argv[0]);
    fuse_args.push_back(const_cast<char *>(mountpoint.c_str()));
    // 可以添加 -f (foreground), -d (debug) 等FUSE标准参数
    fuse_args.push_back((char *)"-f");

    // 5. 启动 FUSE 主循环
    SPDLOG_INFO("Mounting filesystem at {}", mountpoint);
    int ret = fs.run(fuse_args.size(), fuse_args.data());
    SPDLOG_INFO("FuseLLM terminated.");

    return ret;
}
