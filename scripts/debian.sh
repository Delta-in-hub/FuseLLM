#!/bin/bash

# FuseLLM 自动构建脚本
# 适用于 Debian-like 系统

set -e  # 遇到错误立即退出

echo "===== FuseLLM 构建脚本启动 ====="
echo "此脚本将自动完成 FuseLLM 的全部构建过程"

# 1. 检查是否已在 FuseLLM 目录中
if [ ! -f "$(pwd)/CMakeLists.txt" ] || ! grep -q "FuseLLM" "$(pwd)/CMakeLists.txt"; then
  echo "❌ 错误：请在 FuseLLM 项目根目录下运行此脚本"
  exit 1
fi

echo "✅ 已确认在 FuseLLM 项目目录中"

# 2. 初始化和更新子模块
echo "===== 初始化和更新子模块 ====="
if [ -d ".git" ]; then
  git submodule init
  git submodule update --recursive --remote
  echo "✅ 子模块初始化和更新完成"
else
  echo "⚠️ 警告：当前不是 git 仓库，跳过子模块初始化步骤"
fi

# 3. 安装系统依赖
echo "===== 安装系统依赖 ====="
sudo apt update
sudo apt install -y cmake build-essential libfuse3-dev libzmq3-dev libhiredis-dev \
  libcurl4-openssl-dev pkg-config git ninja-build g++ mold doctest-dev \
  nlohmann-json3-dev libtomlplusplus-dev libcxxopts-dev doxygen graphviz

echo "✅ 系统依赖安装完成"

# 4. 从源码编译和安装 spdlog
echo "===== 从源码编译和安装 spdlog ====="
TEMP_DIR=$(mktemp -d)
echo "使用临时目录: $TEMP_DIR"

cd "$TEMP_DIR"
git clone https://github.com/gabime/spdlog.git
cd spdlog/
mkdir build && cd build

echo "配置 spdlog 构建..."
cmake .. -DSPDLOG_BUILD_SHARED=ON -DCMAKE_BUILD_TYPE=Release
echo "编译 spdlog..."
make -j$(nproc)
echo "安装 spdlog (需要 sudo 权限)..."
sudo make install

echo "===== 依赖安装完成 SUCCESS ====="
