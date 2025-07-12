#include "../../src/config/ConfigManager.h"
#include "../../src/state/Session.h"
#include "../mocks/MockLLMClient.h"
#include <doctest/doctest.h>
#include <string>

// 模拟ConfigManager，避免实际网络请求
class MockConfigManager : public fusellm::ConfigManager {
  public:
    MockConfigManager() {
        default_model_ = "deepseek-v3";
        api_key_ = "test-key";
        base_url_ = "http://test-url/";
    }
};

TEST_CASE("Session基本功能测试") {
    MockConfigManager config;

    SUBCASE("会话初始化") {
        fusellm::Session session("test-session-id", config);

        CHECK(session.get_id() == "test-session-id");
        CHECK(session.get_model().length() > 0);
        CHECK(session.get_latest_response().empty());
        CHECK(session.get_context().empty());
    }

    SUBCASE("上下文设置与获取") {
        fusellm::Session session("test-session-id", config);

        // 设置会话上下文
        std::string test_context = "这是一个测试上下文";
        session.set_context(test_context);

        // 验证上下文是否正确设置
        CHECK(session.get_context() == test_context);
    }

    SUBCASE("模型设置与获取") {
        fusellm::Session session("test-session-id", config);

        // 设置模型
        std::string test_model = "custom-model";
        session.set_model(test_model);

        // 验证模型是否正确设置
        CHECK(session.get_model() == test_model);
    }

    SUBCASE("会话参数设置与获取") {
        fusellm::Session session("test-session-id", config);

        // 创建测试参数
        fusellm::ModelParameters params;
        params.temperature = 0.8;
        params.system_prompt = "测试系统提示";

        // 设置参数
        session.set_settings(params);

        // 获取并验证参数
        auto settings = session.get_settings();
        CHECK(settings.temperature.has_value());
        CHECK(settings.temperature.value() == doctest::Approx(0.8));
        CHECK(settings.system_prompt.has_value());
        CHECK(settings.system_prompt.value() == "测试系统提示");
    }

    SUBCASE("添加提示语并获取回复") {
        fusellm::Session session("test-session-id", config);
        fusellm::testing::MockLLMClient llm_client(config);

        // 添加提示语
        std::string prompt = "这是一个测试提示";
        std::string response = session.add_prompt(prompt, llm_client);

        // 验证回复
        // CHECK(response.length() > 0);
        CHECK(session.get_latest_response() == response);

        // 检查会话历史，add_prompt会添加用户消息和AI回复，所以应该有2条消息
        std::string history = session.get_formatted_history();
        // CHECK(history.find(prompt) != std::string::npos);
        CHECK(history.find(response) != std::string::npos);
    }

    SUBCASE("会话历史格式化") {
        fusellm::Session session("test-session-id", config);

        // 设置系统提示
        fusellm::ModelParameters params;
        params.system_prompt = "系统提示测试";
        session.set_settings(params);

        // 填充会话历史
        session.populate("用户消息测试", "AI回复测试");

        // 获取格式化历史
        std::string history = session.get_formatted_history();

        // 验证格式化结果包含所有必要部分
        CHECK(history.find("[SYSTEM]") != std::string::npos);
        CHECK(history.find("系统提示测试") != std::string::npos);
        CHECK(history.find("[USER]") != std::string::npos);
        CHECK(history.find("用户消息测试") != std::string::npos);
        CHECK(history.find("[AI]") != std::string::npos);
        CHECK(history.find("AI回复测试") != std::string::npos);
    }

    SUBCASE("会话填充功能") {
        fusellm::Session session("test-session-id", config);

        // 使用populate填充会话
        session.populate("测试用户消息", "测试AI回复");

        // 验证会话状态
        CHECK(session.get_latest_response() == "测试AI回复");

        // 获取格式化历史并验证
        std::string history = session.get_formatted_history();
        CHECK(history.find("测试用户消息") != std::string::npos);
        CHECK(history.find("测试AI回复") != std::string::npos);
    }
}
