// src/handlers/ConversationsHandler.h
#pragma once
#include "BaseHandler.h"

namespace fusellm {
// 处理 /conversations 目录的所有操作 (mkdir, rmdir, ls) 以及单个会话目录内的所有文件交互
class ConversationsHandler : public BaseHandler {
    // readdir: 列出所有会话
    // mkdir: 创建新会话
    // rmdir: 删除会话
    // read/write: 处理 prompt, context, config 等文件的交互
    // symlink/readlink: 处理 latest 符号链接
};
} 
