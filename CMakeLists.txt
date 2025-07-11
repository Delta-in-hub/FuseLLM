# 建议使用稍新一点的 CMake 版本，很多现代特性在 3.15 后更稳定
cmake_minimum_required(VERSION 3.15)
project(fusellm LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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


# --- 3. 定义可执行文件 ---
add_executable(fusellm
    src/hello.cpp
)

# --- 4. 链接所有依赖项目标 ---
target_link_libraries(fusellm PRIVATE
    # --- 来自子模块的目标 ---
    cppzmq
    redis++::redis++             # redis-plus-plus 创建的目标
    # --- 来自 find_package 的目标 ---
    doctest::doctest
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
    cxxopts::cxxopts

    # --- 底层库 (通常由上面的目标自动传递，但保留可以增加健壮性) ---
    PkgConfig::Hiredis
    PkgConfig::ZeroMQ
    PkgConfig::CURL
)

# --- 5. 设置构建选项 ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(fusellm PRIVATE
        -g
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(fusellm PRIVATE
        -fsanitize=address
    )
endif()