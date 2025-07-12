#include "../../src/handlers/RootHandler.h"
#include <doctest/doctest.h>
#include <sys/stat.h>
#include <vector>
#include <string>

using namespace fusellm;

// 用于捕获 readdir 填充的目录项
struct DirEntry {
    std::string name;
};

// 测试用的 filler 函数
static int test_filler(void* buf, const char* name, const struct stat* stbuf,
                      off_t off, enum fuse_fill_dir_flags flags) {
    auto entries = static_cast<std::vector<DirEntry>*>(buf);
    entries->push_back({name});
    return 0;
}

TEST_CASE("RootHandler基本功能测试") {
    RootHandler handler;
    
    SUBCASE("getattr功能测试") {
        struct stat stbuf;
        
        // 测试根目录
        int res = handler.getattr("/", &stbuf, nullptr);
        CHECK(res == 0);
        CHECK((stbuf.st_mode & S_IFMT) == S_IFDIR);
        CHECK((stbuf.st_mode & 0777) == 0755);
        CHECK(stbuf.st_nlink >= 2);  // 根目录至少有 2 个链接
        
        // 测试一级子目录
        std::vector<const char*> subdirs = {
            "/models", "/config", "/conversations", "/semantic_search"
        };
        
        for (const auto& subdir : subdirs) {
            memset(&stbuf, 0, sizeof(stbuf));
            res = handler.getattr(subdir, &stbuf, nullptr);
            CHECK(res == 0);
            CHECK((stbuf.st_mode & S_IFMT) == S_IFDIR);
            CHECK((stbuf.st_mode & 0777) == 0755);
            CHECK(stbuf.st_nlink == 2);  // 子目录有 2 个链接 (自己和 .)
        }
        
        // 测试不存在的路径
        memset(&stbuf, 0, sizeof(stbuf));
        res = handler.getattr("/nonexistent", &stbuf, nullptr);
        CHECK(res == -ENOENT);
    }
    
    SUBCASE("readdir功能测试") {
        std::vector<DirEntry> entries;
        
        // 测试根目录的 readdir
        int res = handler.readdir("/", &entries, test_filler, 0, nullptr, (fuse_readdir_flags)0);
        CHECK(res == 0);
        
        // 验证根目录条目数量（应该有 ".", ".." 和 4 个子目录）
        CHECK(entries.size() == 6);
        
        // 验证根目录包含预期的条目
        std::vector<std::string> expected_entries = {
            ".", "..", "models", "config", "conversations", "semantic_search"
        };
        
        for (const auto& expected : expected_entries) {
            bool found = false;
            for (const auto& entry : entries) {
                if (entry.name == expected) {
                    found = true;
                    break;
                }
            }
            CHECK(found);
        }
        
        // 测试非根目录的 readdir（应该返回错误）
        entries.clear();
        res = handler.readdir("/models", &entries, test_filler, 0, nullptr, (fuse_readdir_flags)0);
        CHECK(res == -ENOENT);
    }
}
