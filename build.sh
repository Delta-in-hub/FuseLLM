#!/bin/bash

# 当任何命令失败时立即退出脚本
set -e

# --- 设定构建类型 ---
# 默认为 Debug 构建
BUILD_TYPE="Debug"
# 如果第一个参数是 "release"，则切换到 Release 构建
if [[ "$1" == "release" ]]; then
  BUILD_TYPE="Release"
  echo "Release 构建模式已启用。"
else
  echo "默认使用 Debug 构建模式。要进行 Release 构建, 请运行: ./build.sh release"
fi

# --- 可配置变量 ---
# 项目源代码的根目录 (假定此脚本位于项目根目录)
SOURCE_DIR=$(pwd)
# 构建目录的名称
BUILD_DIR="build"

# --- CMake 配置 ---
# 使用 clang/clang++ 编译器, Ninja 作为生成器
# 并通过 CMAKE_*_LINKER_FLAGS 将 "-fuse-ld=mold" 标志传递给链接器
echo "正在为 ${BUILD_TYPE} 模式配置 CMake..."
cmake \
    -S "${SOURCE_DIR}" \
    -B "${BUILD_DIR}" \
    -G "Ninja" \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold"

# --- 执行构建 ---
echo "正在使用 Ninja 进行构建..."
cmake --build "${BUILD_DIR}" --verbose

echo "构建完成！"
echo "产物位于 '${BUILD_DIR}' 目录中。"