cmake_minimum_required(VERSION 3.16)

# 添加单元测试可执行文件
add_executable(fusellm_tests
    # 主测试文件
    test_main.cpp
    
    # fs 模块测试
    fs/test_PathParser.cpp
    
    # config 模块测试
    config/test_ConfigManager.cpp

    # state 模块测试
    state/test_SessionManager.cpp
    state/test_Session.cpp
    
    # handlers 模块测试
    handlers/test_RootHandler.cpp
    handlers/test_ConfigHandler.cpp

    # services 模块测试
    services/test_LLMClient.cpp
    services/test_ZmqClient.cpp
)

# 链接必要的库
target_link_libraries(fusellm_tests PRIVATE
    # 链接主项目库
    fusellmlib
    
    # 外部依赖
    doctest::doctest
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
    cppzmq
    redis++::redis++
    
    # 系统库
    PkgConfig::FUSE
    PkgConfig::Hiredis
    PkgConfig::ZeroMQ
    PkgConfig::CURL
)

target_compile_options(fusellm_tests PRIVATE
    -g
    -fsanitize=address
    -fno-omit-frame-pointer
)

target_link_options(fusellm_tests PRIVATE
    -fsanitize=address
)

# 添加测试
add_test(NAME fusellm_unit_tests COMMAND fusellm_tests)
