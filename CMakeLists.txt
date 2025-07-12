# 建议使用稍新一点的 CMake 版本，很多现代特性在 3.15 后更稳定
cmake_minimum_required(VERSION 3.16)
project(FuseLLM CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 将项目根目录和 external 目录添加到包含路径中，
# 这样可以轻松地 #include "external/Fusepp/Fuse.h" 等文件。
include_directories(${CMAKE_SOURCE_DIR})



# --- 1. 查找底层 C 库和外部依赖 ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED IMPORTED_TARGET fuse3)
pkg_check_modules(ZeroMQ REQUIRED IMPORTED_TARGET libzmq)
pkg_check_modules(Hiredis REQUIRED IMPORTED_TARGET hiredis)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)

find_package(doctest CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)


# --- 2. 添加并构建 Git Submodules ---

# 现在添加子目录，它们会遵循上面的设置
add_subdirectory(external/cppzmq)
add_subdirectory(external/redis-plus-plus)


# --- 3. 创建库文件 ---
add_library(fusellmlib SHARED
    # 所有源代码文件
    src/config/ConfigManager.cpp
    src/fs/FuseLLM.cpp
    src/fs/PathParser.cpp
    src/services/LLMClient.cpp
    src/services/ZmqClient.cpp
    src/state/Session.cpp
    src/state/SessionManager.cpp
    src/handlers/ConfigHandler.cpp
    src/handlers/ConversationsHandler.cpp
    src/handlers/ModelsHandler.cpp
    src/handlers/RootHandler.cpp
    src/handlers/SemanticSearchHandler.cpp
)

# 为支持大文件（超过 2GB），添加此编译定义
target_compile_definitions(fusellmlib PUBLIC _FILE_OFFSET_BITS=64)

# --- 4. 定义可执行文件 ---
add_executable(fusellm
    src/main.cpp
)

# 设置动态库属性
set_target_properties(fusellmlib PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# 为库链接所有依赖
target_link_libraries(fusellmlib PUBLIC
    # --- 来自子模块的目标 ---
    cppzmq
    # redis++::redis++             # redis-plus-plus 创建的目标
    # --- 来自 find_package 的目标 ---
    doctest::doctest
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
    cxxopts::cxxopts

    # --- 底层库 (通常由上面的目标自动传递，但保留可以增加健壮性) ---
    PkgConfig::FUSE
    PkgConfig::Hiredis
    PkgConfig::ZeroMQ
    PkgConfig::CURL
)

# 链接到我们的库
target_link_libraries(fusellm PRIVATE fusellmlib)

# 显式添加 FUSE 的包含目录，确保头文件能被找到
target_include_directories(fusellmlib PUBLIC ${FUSE_INCLUDE_DIRS})

# --- 5. 设置构建选项 ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(fusellmlib PRIVATE
        -g
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(fusellmlib PRIVATE
        -fsanitize=address
    )
    
    target_compile_options(fusellm PRIVATE
        -g
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(fusellm PRIVATE
        -fsanitize=address
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # 在Release模式下保留调试符号，同时保持优化
    target_compile_options(fusellmlib PRIVATE
        -g      # 保留调试符号
        -fno-omit-frame-pointer
        -Ofast     # 优化级别
    )
    
    target_compile_options(fusellm PRIVATE
        -g      # 保留调试符号
        -fno-omit-frame-pointer
        -Ofast     # 优化级别
    )
endif()


# 启用测试支持
enable_testing()

# 添加测试目录
add_subdirectory(test)
