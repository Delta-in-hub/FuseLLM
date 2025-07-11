// src/handlers/SemanticSearchHandler.h
#pragma once
#include "BaseHandler.h"

namespace fusellm {
// 处理 /semantic_search 目录的所有操作 (mkdir, rmdir) 以及 corpus 和 query 文件的逻辑
class SemanticSearchHandler : public BaseHandler {
    // readdir: 列出所有索引
    // mkdir: 创建新索引 (通过ZMQ通知python服务)
    // rmdir: 删除索引
    // write to corpus: 触发文件内容读取和索引构建
    // write to query: 触发语义搜索
    // read from query: 读取上次搜索结果
};
} 
