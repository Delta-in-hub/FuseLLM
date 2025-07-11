// src/handlers/ModelsHandler.h
#pragma once
#include "BaseHandler.h"

namespace fusellm {
// 处理 /models 目录的 ls, cat, echo 操作
class ModelsHandler : public BaseHandler {
    // readdir: 列出可用模型
    // read: 读取模型上次交互的结果
    // write: 发起一次性无状态对话
    // symlink/readlink: 处理 default 符号链接
};
} 
