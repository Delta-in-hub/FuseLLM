// src/handlers/ConfigHandler.h
#pragma once
#include "BaseHandler.h"

namespace fusellm {
// 处理 /config 目录及其下所有 TOML 文件的读写和验证逻辑
class ConfigHandler : public BaseHandler {
    // read: 读取 TOML 配置文件内容
    // write: 验证并写入新的配置
};
} 
