#!/bin/bash

# 当任何命令失败时立即退出脚本
set -e

# --- 配置变量 ---
MOUNT_POINT="/tmp/llm"
# 假设至少有一个模型可用，例如 'deepseek-v3'。
# 请根据你的 API key 可用的模型修改此变量。
# 你可以通过 `ls /tmp/llm/models` 查看可用模型列表。
# 注意：脚本中的某些测试依赖于特定的模型名称。
AVAILABLE_MODEL=$(ls ${MOUNT_POINT}/models | grep -v "default" | head -n 1)

if [ -z "$AVAILABLE_MODEL" ]; then
    echo "错误：在 ${MOUNT_POINT}/models 中找不到任何可用模型。请检查 FuseLLM 配置和 API 连接。"
    exit 1
fi

# --- 辅助函数 ---
# 定义一个函数，用于在每个测试部分前打印一个清晰的标题
print_header() {
    echo ""
    echo "============================================================"
    echo "  $1"
    echo "============================================================"
    echo ""
}


# --- 脚本开始 ---

# 检查挂载点是否存在且不为空
if [ ! -d "$MOUNT_POINT" ] || [ -z "$(ls -A $MOUNT_POINT)" ]; then
    echo "错误: 挂载点 ${MOUNT_POINT} 不存在或为空。"
    echo "请确保 FuseLLM 正在运行并已成功挂载。"
    exit 1
fi

cd "$MOUNT_POINT"

# ============================================================
# 测试 1: 基础文件和目录结构
# ============================================================
print_header "测试 1: 检查基础文件和目录结构"

ls -l .
ls -l models/
ls -l config/
ls -l conversations/
ls -l semantic_search/


# ============================================================
# 测试 2: /models 目录 (无状态交互)
# ============================================================
print_header "测试 2: 无状态模型交互 (/models)"

# 2.1 读取模型的初始状态
echo "--> 2.1 读取模型 '${AVAILABLE_MODEL}' 的初始/上一次回答"
cat "models/${AVAILABLE_MODEL}" || echo "初始读取可能为空，这是正常的。"

# 2.2 发送一个简单的一次性问题
echo "--> 2.2 向 '${AVAILABLE_MODEL}' 发送一个一次性问题"
echo "What is the capital of France?" > "models/${AVAILABLE_MODEL}"

# 2.3 读取模型的回答
echo "--> 2.3 读取模型的回答"
cat "models/${AVAILABLE_MODEL}"

# 2.4 验证无状态交互是否已自动存档到会话中
print_header "测试 2.4: 验证无状态交互的自动存档"
ls -l conversations/


# ============================================================
# 测试 3: /conversations 目录 (有状态交互)
# ============================================================
print_header "测试 3: 有状态会话管理 (/conversations)"

SESSION_NAME="my_test_chat_$(date +%s)"

# 3.1 创建一个新的会话目录
echo "--> 3.1 创建会话: ${SESSION_NAME}"
mkdir "conversations/${SESSION_NAME}"

# 3.2 检查新会话的内部结构
echo "--> 3.2 检查 '${SESSION_NAME}' 的结构"
ls -l "conversations/${SESSION_NAME}"

# 3.3 在新会话中提问
echo "--> 3.3 在会话中提问第一个问题"
echo "My favorite color is blue. What is my favorite color?" > "conversations/${SESSION_NAME}/llm"

# 3.4 读取回答并检查记忆
echo "--> 3.4 读取回答，检查AI是否记住了信息"
cat "conversations/${SESSION_NAME}/llm"

# 3.5 提问第二个问题，测试上下文关联
echo "--> 3.5 提问第二个问题，测试上下文"
echo "Please recommend a fruit of that color." > "conversations/${SESSION_NAME}/llm"
cat "conversations/${SESSION_NAME}/llm"


# 3.6 查看完整的会话历史
echo "--> 3.6 查看完整历史记录"
cat "conversations/${SESSION_NAME}/history"

# 3.8 测试会话特定的上下文 (context)
echo "--> 3.8 为会话设置临时上下文"
echo "BACKGROUND INFO: The user is a C++ programmer." > "conversations/${SESSION_NAME}/context"
cat "conversations/${SESSION_NAME}/context"
echo "--> 3.9 基于新上下文提问"
echo "Based on the background info, explain 'RAII' simply." > "conversations/${SESSION_NAME}/llm"
cat "conversations/${SESSION_NAME}/llm"

# 3.10 删除会话
echo "--> 3.10 删除会话 '${SESSION_NAME}'"
rmdir "conversations/${SESSION_NAME}"
if [ -d "conversations/${SESSION_NAME}" ]; then
    echo "错误: 删除会话失败！"
    exit 1
fi
echo "会话成功删除。"


# ============================================================
# 测试 4: /config 目录
# ============================================================
print_header "测试 4: 配置管理 (/config)"

# 4.1 读取模型专属配置
echo "--> 4.1 读取模型 '${AVAILABLE_MODEL}' 的配置"
cat "config/${AVAILABLE_MODEL}/settings.toml"
echo ""
# 4.2 更新模型专属配置
echo "--> 4.2 更新模型 '${AVAILABLE_MODEL}' 的 temperature"
echo 'temperature = 1.5' > "config/${AVAILABLE_MODEL}/settings.toml"

# 4.3 验证配置已更新
echo "--> 4.3 验证配置已更新"
cat "config/${AVAILABLE_MODEL}/settings.toml"
echo ""
# 4.4 再次进行一次性提问，观察行为是否更有创意
echo "--> 4.4 在高 temperature 下提问"
echo "Write a very short, creative story about a cat who discovers a secret door." > "models/${AVAILABLE_MODEL}"
cat "models/${AVAILABLE_MODEL}"

# 4.5 恢复配置
echo "--> 4.5 恢复默认配置 (写入一个空文件或有效文件)"
echo 'temperature = 0.7' > "config/${AVAILABLE_MODEL}/settings.toml"
cat "config/${AVAILABLE_MODEL}/settings.toml"


# ============================================================
# 测试 5: /semantic_search 目录
# ============================================================
print_header "测试 5: 语义搜索 (/semantic_search)"
INDEX_NAME="my_docs_index_$(date +%s)"

# 5.1 创建一个新的搜索索引
echo "--> 5.1 创建搜索索引: ${INDEX_NAME}"
mkdir "semantic_search/${INDEX_NAME}"
ls -l semantic_search/

# 5.2 将文档添加到语料库
echo "--> 5.2 向语料库添加文档"
echo "Fuse (Filesystem in Userspace) is an interface for userspace programs to export a filesystem to the Linux kernel." > "semantic_search/${INDEX_NAME}/corpus/fuse_intro.txt"
echo "The main component of a FUSE system is the libfuse library, which acts as a bridge." > "semantic_search/${INDEX_NAME}/corpus/libfuse.txt"
echo "LLMs can be mounted using this technology." > "semantic_search/${INDEX_NAME}/corpus/llm_mount.txt"
echo "--> 等待后台服务建立索引..."

# 5.3 执行一个语义查询
echo "--> 5.3 执行查询: 'What is FUSE?'"
echo "What is FUSE for?" > "semantic_search/${INDEX_NAME}/query"

# 5.4 读取查询结果
echo "--> 5.4 读取查询结果"
cat "semantic_search/${INDEX_NAME}/query"
echo ""
# 5.5 从语料库中删除一个文件
echo "--> 5.5 从语料库中删除一个文件"
rm "semantic_search/${INDEX_NAME}/corpus/libfuse.txt"



# 5.6 再次查询，确认结果已变化
echo "--> 5.6 再次查询，确认结果已变化"
echo "Tell me about libfuse" > "semantic_search/${INDEX_NAME}/query"
cat "semantic_search/${INDEX_NAME}/query"

echo ""

# 5.7 删除整个索引
echo "--> 5.7 删除索引: ${INDEX_NAME}"
rmdir "semantic_search/${INDEX_NAME}"
if [ -d "semantic_search/${INDEX_NAME}" ]; then
    echo "错误: 删除索引失败！"
    exit 1
fi
echo "索引成功删除。"


# --- 测试结束 ---
print_header "所有测试已成功完成！ PASS !"
