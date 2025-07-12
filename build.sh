#!/bin/bash

# 当任何命令失败时立即退出脚本
set -e

# --- 设定构建类型 ---
# 默认为 Debug 构建
BUILD_TYPE="Debug"
BUILD_TESTS=ON

# 处理命令行参数
for arg in "$@"
do
  case $arg in
    release)
      BUILD_TYPE="Release"
      echo "Release 构建模式已启用。"
      ;;
    no-tests)
      BUILD_TESTS=OFF
      echo "不构建测试。"
      ;;
    *)
      # 未知参数，忽略
      ;;
  esac
done

# 如果没有明确指定不构建测试，则显示测试将被构建
if [[ "$BUILD_TESTS" == "ON" ]]; then
  echo "将同时构建测试。要跳过构建测试，请添加参数: no-tests"
fi

echo "使用 ${BUILD_TYPE} 构建模式。"

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
    -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" \
    -DBUILD_TESTING=${BUILD_TESTS}

# --- 执行构建 ---
echo "正在使用 Ninja 进行构建..."
cmake --build "${BUILD_DIR}" --verbose

echo "构建完成！"
echo "产物位于 '${BUILD_DIR}' 目录中。"

# 如果构建了测试，显示如何运行测试的信息
if [[ "$BUILD_TESTS" == "ON" ]]; then
  echo "要运行测试，请执行: ${BUILD_DIR}/test/fusellm_tests"
fi

doxygen Doxyfile > /dev/null