// src/handlers/RootHandler.h
#pragma once
#include "BaseHandler.h"

namespace fusellm {
// 处理根目录 (/) 的 ls 操作
class RootHandler : public BaseHandler {
    // 实现 readdir 来列出 models, config, conversations, semantic_search
    // 其他操作返回错误
};
} 
