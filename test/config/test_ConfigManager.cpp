#include "../../src/config/ConfigManager.h"
#include <doctest/doctest.h>
#include <sstream>
#include <toml++/toml.hpp>

TEST_CASE("ConfigManager基本功能测试") {
    using fusellm::ConfigManager;
    using fusellm::ModelParameters;

    SUBCASE("ModelParameters合并功能测试") {
        // 测试TOML表到ModelParameters的合并
        std::stringstream ss;
        ss << "temperature = 0.8\n"
           << "system_prompt = \"测试系统提示\"\n";

        auto tbl = toml::parse(ss);

        ModelParameters params;
        params.merge(tbl);

        CHECK(params.temperature.has_value());
        CHECK(params.temperature.value() == doctest::Approx(0.8));
        CHECK(params.system_prompt.has_value());
        CHECK(params.system_prompt.value() == "测试系统提示");

        // 测试两个ModelParameters对象的合并
        ModelParameters other;
        other.temperature = 1.0;
        // 系统提示保持为空

        params.merge(other);
        CHECK(params.temperature.value() == doctest::Approx(1.0)); // 被覆盖
        CHECK(params.system_prompt.value() == "测试系统提示");     // 保持不变
    }

    SUBCASE("ModelParameters验证功能测试") {
        // 有效配置
        std::stringstream valid_ss;
        valid_ss << "temperature = 0.8\n"
                 << "system_prompt = \"有效的系统提示\"\n";
        auto valid_tbl = toml::parse(valid_ss);

        CHECK(ModelParameters::validate_model_params_table(valid_tbl));

        // 无效温度范围
        std::stringstream invalid_temp_ss;
        invalid_temp_ss << "temperature = 2.5\n"; // 超出有效范围
        auto invalid_temp_tbl = toml::parse(invalid_temp_ss);

        CHECK_FALSE(
            ModelParameters::validate_model_params_table(invalid_temp_tbl));

        // 无效系统提示类型
        std::stringstream invalid_prompt_ss;
        invalid_prompt_ss << "system_prompt = 123\n"; // 应该是字符串

        // toml++ 实际上会将数字123解析为一个整数节点，而非字符串节点
        // 所以我们应该检查validate_model_params_table是否能正确识别并拒绝这种情况
        try {
            auto invalid_prompt_tbl = toml::parse(invalid_prompt_ss);
            // 如果解析成功，应该检查验证结果
            CHECK_FALSE(ModelParameters::validate_model_params_table(
                invalid_prompt_tbl));
        } catch (const toml::parse_error &) {
            // 如果解析失败，也是符合预期的
            CHECK(true);
        }
    }

    SUBCASE("ConfigManager模型参数管理测试") {
        ConfigManager config;

        // 为特定模型更新参数
        std::stringstream model_ss;
        model_ss << "temperature = 0.9\n"
                 << "system_prompt = \"特定模型的系统提示\"\n";
        auto model_tbl = toml::parse(model_ss);

        CHECK(config.update_model_params("test-model", model_tbl));

        // 获取并检查模型参数
        auto params = config.get_model_params("test-model");
        CHECK(params.temperature.has_value());
        CHECK(params.temperature.value() == doctest::Approx(0.9));
        CHECK(params.system_prompt.has_value());
        CHECK(params.system_prompt.value() == "特定模型的系统提示");

        // 检查不存在的模型 - 应该返回全局参数
        auto default_params = config.get_model_params("non-existent-model");
        // 由于我们没有设置全局参数，这些值应该都是nullopt
        CHECK_FALSE(default_params.temperature.has_value());
        // 但根据ConfigManager的构造函数，系统提示可能有默认值
    }
}
