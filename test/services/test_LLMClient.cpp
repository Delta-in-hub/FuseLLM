#include "../../src/config/ConfigManager.h"
#include "../../src/services/LLMClient.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

// 由于LLMClient依赖外部OpenAI API，我们需要创建一个测试专用的简化版本
// 这里我们只测试那些不直接依赖网络请求的部分

// 访问私有方法进行测试的辅助类
class TestLLMClient : public fusellm::LLMClient {
  public:
    explicit TestLLMClient(const fusellm::ConfigManager &config_manager)
        : fusellm::LLMClient(config_manager) {}

    // 暴露私有方法用于测试
    static std::string public_role_to_string(fusellm::Message::Role role) {
        return role_to_string(role);
    }

    static nlohmann::json
    public_build_request_json(std::string_view model_name,
                              const fusellm::ModelParameters &ms,
                              const nlohmann::json &messages) {
        return build_request_json(model_name, ms, messages);
    }

    static std::string
    public_extract_content_from_response(const nlohmann::json &response_json) {
        return extract_content_from_response(response_json);
    }
};

TEST_CASE("LLMClient基本功能测试") {
    // 注意：这些测试不会实际调用OpenAI API

    SUBCASE("Role枚举转换到字符串") {
        CHECK(TestLLMClient::public_role_to_string(
                  fusellm::Message::Role::System) == "system");
        CHECK(TestLLMClient::public_role_to_string(
                  fusellm::Message::Role::User) == "user");
        CHECK(TestLLMClient::public_role_to_string(
                  fusellm::Message::Role::AI) == "assistant");
    }

    SUBCASE("构建请求JSON") {
        // 创建测试用ModelParameters
        fusellm::ModelParameters params;
        params.temperature = 0.7;
        params.system_prompt = "这是一个系统提示";

        // 创建测试用消息
        nlohmann::json messages = nlohmann::json::array(
            {{{"role", "system"}, {"content", "系统消息"}},
             {{"role", "user"}, {"content", "用户消息"}}});

        // 调用测试方法
        nlohmann::json request = TestLLMClient::public_build_request_json(
            "test-model", params, messages);

        // 验证结果
        CHECK(request["model"] == "test-model");
        CHECK(request["temperature"] == doctest::Approx(0.7));
        CHECK(request["messages"].size() == 2);
        CHECK(request["messages"][0]["role"] == "system");
        CHECK(request["messages"][0]["content"] == "系统消息");
        CHECK(request["messages"][1]["role"] == "user");
        CHECK(request["messages"][1]["content"] == "用户消息");
    }

    SUBCASE("从响应中提取内容") {
        // 创建模拟的API响应JSON
        nlohmann::json response = {
            {"id", "test-id"},
            {"object", "chat.completion"},
            {"created", 1625097678},
            {"model", "test-model"},
            {"choices",
             {{{"message",
                {{"role", "assistant"}, {"content", "这是助手的回复"}}},
               {"finish_reason", "stop"},
               {"index", 0}}}}};

        // 调用测试方法
        std::string content =
            TestLLMClient::public_extract_content_from_response(response);

        // 验证结果
        CHECK(content == "这是助手的回复");

        // 测试错误情况
        nlohmann::json empty_response = {{"object", "chat.completion"}};
        std::string empty_content =
            TestLLMClient::public_extract_content_from_response(empty_response);
        CHECK(empty_content.empty());
    }

    // 注意：完整测试应当包含对简单查询和会话查询的测试
    // 但这需要模拟OpenAI API的响应，这超出了基本单元测试的范围
    // 下面是如何扩展这些测试的建议：
    /*
    SUBCASE("简单查询测试 - 需要模拟API") {
        // 1. 创建一个模拟的ConfigManager
        // 2. 创建一个特殊版本的LLMClient，它会覆盖网络请求方法
        // 3. 使用预定义的模拟响应测试simple_query方法
    }

    SUBCASE("会话查询测试 - 需要模拟API") {
        // 类似的方法测试conversation_query
    }
    */
}
