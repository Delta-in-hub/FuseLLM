#include "../../src/fs/PathParser.h"
#include <doctest/doctest.h>

TEST_CASE("PathParser基本功能测试") {
    using fusellm::PathParser;
    using fusellm::PathType;

    SUBCASE("根目录解析") { CHECK(PathParser::parse("/") == PathType::Root); }

    SUBCASE("模型路径解析") {
        CHECK(PathParser::parse("/models") == PathType::Models);
        CHECK(PathParser::parse("/models/gpt-4") == PathType::Models);
    }

    SUBCASE("配置路径解析") {
        CHECK(PathParser::parse("/config") == PathType::Config);
        CHECK(PathParser::parse("/config/settings.toml") == PathType::Config);
        CHECK(PathParser::parse("/config/models/gpt-4/settings.toml") ==
              PathType::Config);
    }

    SUBCASE("会话路径解析") {
        CHECK(PathParser::parse("/conversations") == PathType::Conversations);
        CHECK(PathParser::parse("/conversations/latest") ==
              PathType::Conversations);
        CHECK(PathParser::parse("/conversations/session_123") ==
              PathType::Conversations);
        CHECK(PathParser::parse("/conversations/session_123/history") ==
              PathType::Conversations);
    }

    SUBCASE("语义搜索路径解析") {
        CHECK(PathParser::parse("/semantic_search") ==
              PathType::SemanticSearch);
        CHECK(PathParser::parse("/semantic_search/my_index") ==
              PathType::SemanticSearch);
        CHECK(PathParser::parse("/semantic_search/my_index/corpus/doc.txt") ==
              PathType::SemanticSearch);
    }

    SUBCASE("其他路径解析") {
        CHECK(PathParser::parse("/unknown") == PathType::Other);
        CHECK(PathParser::parse("") == PathType::Other);
    }
}
