// src/main.cpp
#include "fs/FuseLLM.h"
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. 设置日志
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting FuseLLM...");

    // 2. 解析命令行参数 (如 mountpoint, config file)
    cxxopts::Options options("FuseLLM", "Mount your LLM as a filesystem.");
    options.add_options()
        ("m,mountpoint", "Mount point for the filesystem", cxxopts::value<std::string>())
        ("c,config", "Path to global configuration file", cxxopts::value<std::string>()->default_value("config.toml"))
        ("h,help", "Print usage");
    
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (!result.count("mountpoint")) {
        spdlog::error("Mount point is required.");
        std::cout << options.help() << std::endl;
        return 1;
    }

    std::string mountpoint = result["mountpoint"].as<std::string>();

    // 3. 初始化 FuseLLM 实例
    fusellm::FuseLLM fs;

    // 4. 准备 FUSE 参数
    // fusepp 会处理大部分参数，但我们需要把我们的 mountpoint 加进去
    std::vector<char*> fuse_args;
    fuse_args.push_back(argv[0]);
    fuse_args.push_back(const_cast<char*>(mountpoint.c_str()));
    // 可以添加 -f (foreground), -d (debug) 等FUSE标准参数
    fuse_args.push_back((char*)"-f");

    // 5. 启动 FUSE 主循环
    spdlog::info("Mounting filesystem at {}", mountpoint);
    int ret = fs.run(fuse_args.size(), fuse_args.data());
    spdlog::info("FuseLLM terminated.");

    return ret;
}
