#include "../../src/config/ConfigManager.h"
#include "../../src/handlers/ConfigHandler.h"
#include "../mocks/MockLLMClient.h"
#include <doctest/doctest.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace fusellm;

// 测试用的 filler 函数
static int test_filler(void *buf, const char *name, const struct stat *stbuf,
                       off_t off, enum fuse_fill_dir_flags flags) {
    auto entries = static_cast<std::vector<std::string> *>(buf);
    entries->push_back(name);
    return 0;
}

TEST_CASE("ConfigHandler基本功能测试") {
    // 准备测试环境
    ConfigManager config_manager;
    testing::MockLLMClient mock_client(config_manager);
    ConfigHandler handler(config_manager, mock_client);

    // 设置一些测试数据
    std::stringstream model_ss;
    model_ss << "temperature = 0.7\n"
             << "system_prompt = \"测试提示\"\n";
    auto model_tbl = toml::parse(model_ss);
    config_manager.update_model_params("model-1", model_tbl);

    SUBCASE("getattr功能测试") {
        struct stat stbuf;

        // 测试 /config 目录
        int res = handler.getattr("/config", &stbuf, nullptr);
        CHECK(res == 0);
        CHECK((stbuf.st_mode & S_IFMT) == S_IFDIR);

        // 测试模型目录
        res = handler.getattr("/config/model-1", &stbuf, nullptr);
        CHECK(res == 0);
        CHECK((stbuf.st_mode & S_IFMT) == S_IFDIR);

        // 测试默认模型目录
        res = handler.getattr("/config/default", &stbuf, nullptr);
        CHECK(res == 0);
        CHECK((stbuf.st_mode & S_IFMT) == S_IFDIR);

        // 测试设置文件
        res = handler.getattr("/config/model-1/settings.toml", &stbuf, nullptr);
        CHECK(res == 0);
        CHECK((stbuf.st_mode & S_IFMT) == S_IFREG);

        // 测试不存在的路径
        res = handler.getattr("/config/nonexistent", &stbuf, nullptr);
        CHECK(res == -ENOENT);
    }

    SUBCASE("readdir功能测试") {
        std::vector<std::string> entries;

        // 测试 /config 目录的 readdir
        int res = handler.readdir("/config", &entries, test_filler, 0, nullptr,
                                  (fuse_readdir_flags)0);
        CHECK(res == 0);

        // 验证目录条目包含 ".", "..", "default" 和所有模型
        CHECK(entries.size() == 7);

        // 清除条目列表并测试模型目录的 readdir
        entries.clear();
        res = handler.readdir("/config/model-1", &entries, test_filler, 0,
                              nullptr, (fuse_readdir_flags)0);
        CHECK(res == 0);
        CHECK(entries.size() == 3); // ".", "..", "settings.toml"
    }

    SUBCASE("read功能测试") {
        // 准备读取缓冲区
        char buf[1024] = {0};
        const size_t buf_size = sizeof(buf);

        // 读取模型设置文件
        int bytes_read = handler.read("/config/model-1/settings.toml", buf,
                                      buf_size, 0, nullptr);
        CHECK(bytes_read > 0);

        // 验证读取的内容
        std::string content(buf, bytes_read);
        CHECK(content.find("temperature = 0.7") != std::string::npos);
        CHECK(content.find("system_prompt =") != std::string::npos);

        // 测试偏移读取
        memset(buf, 0, buf_size);
        bytes_read = handler.read("/config/model-1/settings.toml", buf,
                                  buf_size, 10, nullptr);
        CHECK(bytes_read > 0);
        CHECK(bytes_read < buf_size);

        // 测试无效路径
        bytes_read = handler.read("/config/nonexistent/settings.toml", buf,
                                  buf_size, 0, nullptr);
        CHECK(bytes_read == -ENOENT);
    }

    SUBCASE("write功能测试") {
        // 准备写入数据
        std::string write_data =
            "temperature = 0.8\nsystem_prompt = \"更新的提示\"\n";

        // 写入模型设置文件
        int bytes_written =
            handler.write("/config/model-1/settings.toml", write_data.c_str(),
                          write_data.size(), 0, nullptr);
        CHECK(bytes_written == write_data.size());

        // 验证写入后的数据是否更新
        char read_buf[1024] = {0};
        int bytes_read = handler.read("/config/model-1/settings.toml", read_buf,
                                      sizeof(read_buf), 0, nullptr);
        CHECK(bytes_read > 0);

        std::string updated_content(read_buf, bytes_read);
        CHECK(updated_content.find("temperature = 0.8") != std::string::npos);
        CHECK(updated_content.find("更新的提示") != std::string::npos);

        // 测试偏移写入（应该失败，因为不支持部分写入）
        bytes_written =
            handler.write("/config/model-1/settings.toml", write_data.c_str(),
                          write_data.size(), 10, nullptr);
        CHECK(bytes_written == -EPERM);

        // 测试写入无效的TOML内容
        std::string invalid_toml = "temperature = invalid\n";
        bytes_written =
            handler.write("/config/model-1/settings.toml", invalid_toml.c_str(),
                          invalid_toml.size(), 0, nullptr);
        CHECK(bytes_written == -EINVAL);
    }
}
