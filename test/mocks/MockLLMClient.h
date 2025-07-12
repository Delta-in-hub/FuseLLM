#pragma once

#include "../../src/services/LLMClient.h"
#include "../../src/config/ConfigManager.h"
#include <string>
#include <vector>

namespace fusellm {
namespace testing {

/**
 * @class MockLLMClient
 * @brief 用于测试的 LLMClient 模拟类
 * 
 * 这个类模拟 LLMClient 的行为而不需要真正的网络请求。
 * 用于单元测试和集成测试。
 */
class MockLLMClient : public fusellm::LLMClient {
public:
    /**
     * @brief 构造一个 MockLLMClient 实例
     * @param config_manager 配置管理器的引用
     */
    explicit MockLLMClient(const fusellm::ConfigManager &config_manager)
        : fusellm::LLMClient(config_manager) {
        // 初始化模拟的模型列表
        model_list = {"model-1", "model-2", "model-3", "test-model"};
    }
    
    /**
     * @brief 重载会话查询方法，返回固定响应
     */
    std::string conversation_query(std::string_view model_name,
                                   const fusellm::ConfigManager &config_manager,
                                   const fusellm::Conversation &conversation) {
        last_model = std::string(model_name);
        last_conversation = conversation;
        return "这是一个模拟的AI回复";
    }
    
    // 存储最后一次查询的模型名称和对话，用于验证
    std::string last_model;
    fusellm::Conversation last_conversation;
};

} // namespace testing
} // namespace fusellm
