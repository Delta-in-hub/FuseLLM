#include "../../src/state/SessionManager.h"
#include <doctest/doctest.h>
#include <vector>
#include <string>
#include <algorithm>

using namespace fusellm;

// 创建ConfigManager的模拟实现，仅用于测试
class MockConfigManager : public ConfigManager {
public:
    MockConfigManager() : ConfigManager() {}
};

TEST_CASE("SessionManager基本功能测试") {
    // 准备一个ConfigManager实例以传递给SessionManager
    MockConfigManager config;
    
    SUBCASE("创建和查找会话") {
        SessionManager manager(config);
        
        // 使用指定ID创建会话
        std::string test_id = "test-session-1";
        auto session = manager.create_session(test_id);
        
        // 确保会话被正确创建
        CHECK(session != nullptr);
        CHECK(session->get_id() == test_id);
        
        // 查找已创建的会话
        auto found_session = manager.find_session(test_id);
        CHECK(found_session != nullptr);
        CHECK(found_session->get_id() == test_id);
        
        // 尝试创建具有重复ID的会话
        auto duplicate_session = manager.create_session(test_id);
        CHECK(duplicate_session == nullptr);
        
        // 查找不存在的会话
        auto non_existent_session = manager.find_session("non-existent");
        CHECK(non_existent_session == nullptr);
    }
    
    SUBCASE("自动生成会话ID") {
        SessionManager manager(config);
        
        // 创建具有自动生成ID的会话
        auto session1 = manager.create_session_with_auto_id();
        auto session2 = manager.create_session_with_auto_id();
        
        // 确保会话被正确创建且ID不同
        CHECK(session1 != nullptr);
        CHECK(session2 != nullptr);
        CHECK(session1->get_id() != session2->get_id());
        
        // 验证自动生成的ID是数字字符串（从1000开始）
        CHECK(std::stol(session1->get_id()) >= 1000);
        CHECK(std::stol(session2->get_id()) >= 1000);
    }
    
    SUBCASE("列出和删除会话") {
        SessionManager manager(config);
        
        // 创建多个会话
        manager.create_session("session-1");
        manager.create_session("session-2");
        manager.create_session("session-3");
        
        // 列出所有会话
        auto sessions = manager.list_sessions();
        CHECK(sessions.size() == 3);
        CHECK(std::find(sessions.begin(), sessions.end(), "session-1") != sessions.end());
        CHECK(std::find(sessions.begin(), sessions.end(), "session-2") != sessions.end());
        CHECK(std::find(sessions.begin(), sessions.end(), "session-3") != sessions.end());
        
        // 删除一个会话
        bool removed = manager.remove_session("session-2");
        CHECK(removed);
        
        // 确认会话已被删除
        sessions = manager.list_sessions();
        CHECK(sessions.size() == 2);
        CHECK(std::find(sessions.begin(), sessions.end(), "session-1") != sessions.end());
        CHECK(std::find(sessions.begin(), sessions.end(), "session-2") == sessions.end());
        CHECK(std::find(sessions.begin(), sessions.end(), "session-3") != sessions.end());
        
        // 尝试删除不存在的会话
        removed = manager.remove_session("non-existent");
        CHECK_FALSE(removed);
    }
    
    SUBCASE("最近使用的会话管理") {
        SessionManager manager(config);
        
        // 初始状态下，最近使用的会话ID应为空
        CHECK(manager.get_latest_session_id().empty());
        
        // 设置最近使用的会话ID
        std::string latest_id = "latest-session";
        manager.set_latest_session_id(latest_id);
        CHECK(manager.get_latest_session_id() == latest_id);
        
        // 删除最近使用的会话时，最近会话ID应被清除
        manager.create_session(latest_id);
        manager.remove_session(latest_id);
        CHECK(manager.get_latest_session_id().empty());
    }
    
    SUBCASE("线程安全性测试") {
        // 注意：这不是完全的线程安全测试，只是基本验证
        SessionManager manager(config);
        
        // 创建一些会话
        for (int i = 0; i < 5; i++) {
            std::string id = "session-" + std::to_string(i);
            auto session = manager.create_session(id);
            CHECK(session != nullptr);
        }
        
        // 列出会话并检查数量
        auto sessions = manager.list_sessions();
        CHECK(sessions.size() == 5);
    }
}
