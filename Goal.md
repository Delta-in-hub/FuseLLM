# **FuseLLM**

### Mount your LLM.

*Everything is a file. Even the LLM.*



1. **FuseLLM** — This is **WHAT** it is.
2. **Mount your LLM.** — This is **HOW** you use it.
3. **Everything is a file. Even the LLM.** — This is **WHY** it exists.





### **1. 核心设计哲学**

*   **一切皆文件**: 将与 LLM 的交互（如会话、提问、配置、上下文管理）以及对文件系统本身的管理，全部映射为文件和目录的读写操作。这使得任何标准的命令行工具 (`ls`, `cat`, `echo`, `grep`, `find`) 都能自然地与 LLM 互动。
*   **LLM 即驱动**: 文件系统本身不存储常规数据，它是一个转换层或“驱动程序”。写入某些文件会触发对 LLM API 的调用。
*   **层次化与情景化**: 使用目录来组织和隔离不同的交互会话（conversations）。每个会话拥有独立的上下文、历史记录和配置，类似于 `/proc` 下的每个进程目录 (`/proc/<pid>/`) 都包含该进程的独立信息。

### **2. 文件系统结构（挂载点：/mnt/fusellm）**

```
/mnt/fusellm
- /models
  ：目录，列出可用 LLM 模型。
  - /models/default
  - /models/gpt-3.5：文件， 与 GPT-3.5 模型交互。 写是询问LLM, 读是LLM最新一次的回复。每次写，创建一个新的 conversation
  - /models/gpt-4：GPT-4 模型交互。
  - /config/model/*/setting：某个模型的配置文件， TOML 格式.
├── config         # [目录] 配置
---- settings.toml  ; 默认配置 ， 写的时候，后端要进行格式验证，否则返回错误。
│   ├── models/
---------------/gpt3.5/settings.toml       模型的配置， 覆盖全局默认配置
├── conversations       # [目录] 存放所有交互会话
------- latest       最新的一个会话， 注意特殊情况， 目录为空
│   ├── <session_id_1> # [目录] 一个独立的会话，类似 PID 分配的方式， 整个目录可以被删除
│   │   ├── prompt     # [文件] 核心交互文件。写入触发提问，读取获得回答。 
│   │   ├── history    # [文件] 只读，包含此会话的完整问答历史
│   │   ├── context    # [文件] 可读写，代表当前会话的上下文或"记忆"
│   │   └── config     # [目录] 此会话的专属配置
│   │       ├── model  # [文件] 覆盖全局模型设置
│   │       ├── system_prompt # [文件] 设置此会话的系统提示/角色
│   │       └── settings.toml   
│   │
│   └── <session_id_2> # [目录] 另一个独立的会话
│       └── ...
│
└── semantic_search # [目录] 语义搜索工具
	--- default
        ├── corpus   # [目录] 文档语料库。用户可将文件放入此处
        └── query    # [文件] 查询接口。写入问题，读取返回最相关的文档片段
	--- <idxxx>
        ├── corpus   # [目录] 文档语料库。用户可将文件放入此处
        └── query    # [文件] 查询接口。写入问题，读取返回最相关的文档片段
```

### **3. 文件/目录操作详解**

```
本节详细说明了在 FuseLLM 文件系统中，针对每个文件和目录执行标准命令 (如 `ls`, `cat`, `echo`, `mkdir`, `rm`) 时，所期望的具体行为和后端逻辑。

---

#### **根目录 (`/mnt/fusellm`)**

*   **`ls -l /mnt/fusellm`**
    *   **行为**: 列出顶层目录。
    *   **输出**: `models`, `config`, `conversations`, `semantic_search`。

---

#### **模型目录 (`/models`)**

此目录提供对已配置模型的直接、无状态访问。

*   **`ls -l /models`**
    *   **行为**: 列出所有后端配置中可用的 LLM 模型。
    *   **输出**: 类似 `default` (符号链接), `gpt-3.5`, `gpt-4`, `llama3` 等文件。
    *   **实现细节**: `default` 是一个指向默认模型的符号链接，可以通过 `ln -sf gpt-4 /models/default` 命令来更改。

*   **`cat /models/gpt-4`**
    *   **行为**: 读取此模型**上一次无状态交互**的回答。如果从未进行过交互，则返回一条状态信息。
    *   **输出示例**: `I am GPT-4, ready for your request.` 或上一次 `echo` 请求的答案。
    *   **用途**: 快速检查模型可用性或获取上次简单查询的结果。

*   **`echo "Translate 'hello world' to French" > /models/gpt-4`**
    *   **行为**: 发起一次**无状态、一次性**的对话。
    *   **后端逻辑**:
        1.  接收到写入请求。
        2.  使用写入的内容作为 `prompt`。
        3.  调用 `gpt-4` 模型 API。
        4.  阻塞操作，直到收到 LLM 的完整回复。
        5.  将回复存储在与该模型文件关联的临时内存区域中。
        6.  写入操作成功返回。
    *   **用途**: “开箱即用”的快速问答，无需创建完整的会话。非常适合脚本化的一次性任务。

*   **`rm /models/gpt-4` 或 `touch /models/new-model`**
    *   **行为**: 操作失败。
    *   **返回**: `Permission denied` 或 `Read-only file system`。模型列表由系统配置决定，不能由用户在文件系统层面直接增删。

---

#### **配置目录 (`/config`)**

*   **`/config/settings.toml` (全局配置)**
    *   `cat settings.toml`: 读取并以 TOML 格式返回当前的全局配置。
    *   `echo "[model]\ntemperature = 0.8" > settings.toml`:
        1.  后端捕获写入内容。
        2.  **验证**: 使用 `toml` 库解析内容，并根据预设的配置结构进行验证（例如，`temperature` 必须是 0 到 2 之间的浮点数）。
        3.  **失败**: 如果验证失败，写入操作将失败，并返回 `EINVAL` (Invalid argument) 或 `EIO` 错误，错误信息可以包含验证失败的原因。
        4.  **成功**: 如果验证成功，则更新后端的全局配置状态。

*   **`/config/models/<model_name>/settings.toml` (模型专属配置)**
    *   行为与全局配置类似，但其作用域仅限于 `<model_name>`。此处的设置会**覆盖** `/config/settings.toml` 中的同名设置。
    *   例如 `cat /config/models/gpt-4/settings.toml` 可能显示 `temperature = 1.2`，即使全局设置是 `0.8`。

---

#### **会话目录 (`/conversations`)**

管理有状态、持续性的对话。

*   **`ls -l /conversations`**
    *   **行为**: 列出所有活动的会话目录和 `latest` 符号链接。
    *   **输出**: `1001`, `1002`, `my-project-chat`, `latest -> my-project-chat`。

*   **`mkdir /conversations/my-new-chat`**
    *   **行为**: 创建一个新的、持久化的会话。
    *   **后端逻辑**:
        1.  分配一个新的会话状态对象（包含空的历史记录、上下文等）。
        2.  使用 `my-new-chat` 作为其标识符。
        3.  在文件系统视图中创建 `/conversations/my-new-chat` 目录及其内部结构 (`prompt`, `history` 等文件)。

*   **`rmdir /conversations/my-new-chat` 或 `rm -r /conversations/my-new-chat`**
    *   **行为**: 删除一个会话及其所有历史记录。
    *   **后端逻辑**: 从内存中彻底清除与该会话 ID 相关的所有状态（历史、上下文、配置）。这是一个不可逆操作。

*   **`latest` (符号链接)**
    *   这是一个动态管理的符号链接，**永远指向最近有过交互（特指写入 `prompt` 文件）的会话目录**。
    *   **用途**: 极大地简化了工作流，用户可以随时通过 `cd /mnt/fusellm/conversations/latest` 进入最新的工作环境。

---

#### **单个会话目录 (`/conversations/<session_id>/`)**

*   **`prompt` (文件)**
    *   `echo "My question is..." > prompt`: **核心交互操作**。
        1.  将写入的内容作为用户的新问题。
        2.  将其追加到会话的内部历史记录中。
        3.  构建完整的请求体（包含 `system_prompt`, `context`, 和 `history`）。
        4.  向该会话配置的 LLM (`./config/model` 文件指定) 发送 API 请求。
        5.  操作会阻塞，直到 LLM 返回响应。
        6.  将 LLM 的回答存储在会话的“最新回答”字段中，并追加到历史记录。
    *   `cat prompt`: 读取并返回该会话**最新的 LLM 回答**。如果一个 `echo` 操作正在进行中，`cat` 将会阻塞直到回答准备就绪。

*   **`history` (只读文件)**
    *   `cat history`: 读取该会话自创建以来的**完整对话历史**。
    *   **输出格式**:
        ```
        [SYSTEM]
        You are a helpful assistant.

        [USER]
        What is FUSE?

        [AI]
        FUSE stands for Filesystem in Userspace...
        ```
    *   `echo "..." > history`: 操作失败，返回 `Read-only file system`。

*   **`context` (读写文件)**
    *   `cat context`: 读取当前为会话设置的额外上下文。
    *   `echo "Use this document as reference: ..." > context`: **覆盖**当前上下文。
    *   `cat doc.txt >> context`: **追加**内容到当前上下文。
    *   **用途**: 为 LLM 提供临时的、不计入永久 `history` 的背景信息，例如粘贴一段代码或一篇文章让 LLM 分析。

*   **`config/` (目录)**: 会话专属配置，拥有最高优先级。
    *   `config/model` (文件):
        *   `cat model`: 显示当前会话使用的模型，如 `gpt-4`。
        *   `echo "llama3" > model`: 将当前会话的 LLM 切换为 `llama3`。
    *   `config/system_prompt` (文件):
        *   `cat system_prompt`: 显示当前会话的系统提示/角色设定。
        *   `echo "You are a succinct technical writer." > system_prompt`: 为当前会话设定新的角色。
    *   `config/settings.toml` (文件):
        *   行为与全局/模型配置一样，但仅对当前会话生效，优先级最高。例如，可以在这里临时为某个会话开启更高的 `temperature` 以获得更有创意的回答。

---

#### **语义搜索目录 (`/semantic_search`)**

提供基于向量的文档搜索能力。

*   **`ls -l /semantic_search`**
    *   **行为**: 列出所有已创建的搜索索引。
    *   **输出**: `default`, `my-codebase`, `project-docs`。

*   **`mkdir /semantic_search/my-api-docs`**
    *   **行为**: 创建一个新的、空的语义搜索索引。
    *   **后端逻辑**: 初始化一个空的向量数据库/索引实例，并将其与 `my-api-docs` 这个名字关联。

*   **`rmdir /semantic_search/my-api-docs`**
    *   **行为**: 删除整个索引及其包含的所有数据。

*   **索引内部 (`/semantic_search/<index_id>/`)**
    *   **`corpus/` (目录)**:
        *   `cp ~/doc.md ./corpus/` 或 `echo "text chunk" > ./corpus/new_file.txt`: **触发索引构建**。
            1.  后端监测到 `corpus` 目录中有新文件写入或创建。
            2.  读取文件内容。
            3.  （可选）将内容分块 (chunking)。
            4.  使用预配置的嵌入模型为每个块生成向量嵌入。
            5.  将文本块和其向量存入与 `<index_id>` 关联的向量数据库中。
        *   `rm ./corpus/doc.md`: 从索引中删除与此文件相关的所有向量和数据。
    *   **`query` (文件)**:
        *   `echo "How to use the login endpoint?" > query`: **执行语义搜索**。
            1.  后端捕获写入的查询字符串。
            2.  为查询字符串生成向量嵌入。
            3.  在 `<index_id>` 对应的向量数据库中执行相似性搜索。
            4.  获取排名最前的 N 个结果（文档片段）。
            5.  将格式化后的结果存放在该文件的读取缓冲区中。
        *   `cat query`: 读取并返回**上一次搜索的结果**。
        *   **输出格式**:
            ```
            --- Result 1/3 (Score: 0.92) ---
            Source: /corpus/auth-api.md
            Content: ... a valid JWT token must be provided in the Authorization header to access the /login endpoint ...

            --- Result 2/3 (Score: 0.88) ---
            Source: /corpus/examples.txt
            Content: ... example of logging in: curl -X POST /login -d '{"user": "..."}' ...
            ```
```









### **4. 技术实现 (Rust 实现层面)**



使用Linux环境，使用 RUST 语言，通过 FUSE 实现以上功能。

semantic_search 部分，通过 RUST 通过 zeromq 的 ipc 和 python 通信， python 使用llama_index实现。

要求符合现代 RUST 开发的最佳实践。



- TDD 测试驱动开发
  - https://doc.rust-lang.org/rust-by-example/testing.html
- 推荐可以使用的库
  - https://docs.rs/fuser/latest/fuser/
  - https://docs.rs/async-openai/latest/async_openai/
  - https://docs.rs/async-std/latest/async_std/
  - https://docs.rs/toml/latest/toml/
  - https://docs.rs/redis/latest/redis/
  - https://docs.rs/crate/zmq/latest
  - https://docs.rs/serde_json/latest/serde_json/

*   **状态管理**: 会话状态（历史、配置）需要被安全地管理。选择内存存储（简单）来保存状态。
*   **并发控制**: 多个进程可能同时访问文件系统。需要为每个会话的关键操作（如写入 `prompt`）实现锁或队列机制，以防止竞争条件。
*   **错误处理**: LLM API 调用失败、网络问题等都应被妥善处理，并向上层应用返回标准的文件系统错误码，例如 `EIO` (Input/output error)。
*   **安全性**: API 密钥和其他敏感配置绝不能硬编码。应通过环境变量或安全的配置文件在文件系统挂载时提供。





#### 目录结构

```

fusellm/
├── Cargo.toml          # Project dependencies (fuser, async-std, async-openai, etc.) and metadata
├── semantic_search_service/      # Python backend for semantic search
│   ├── requirements.txt  # Python dependencies (llama-index, zeromq, etc.)
│   └── service.py      # The ZMQ server implementing the search logic
├── src/
│   ├── main.rs         # Entry point: parses arguments, sets up logging, mounts the FS
│   ├── lib.rs          # Main library module, exports the Filesystem struct
│   ├── config.rs       # Defines structs for TOML configuration (using serde)
│   ├── state.rs        # Core state management: structs for FilesystemState, Conversation, SearchIndex
│   ├── llm_api.rs      # Abstraction for communicating with LLM APIs (e.g., OpenAI)
│   │
│   ├── semantic/       # Module for semantic search communication
│   │   ├── mod.rs
│   │   └── client.rs   # ZMQ client to talk to the semantic_search_service/service.py
│   │
│   └── fs/             # The main FUSE Filesystem implementation
│       ├── mod.rs      # The main Filesystem struct and its `impl Filesystem for FuseLlm`
│       ├── constants.rs  # Inode numbers, file names, etc.
│       └── handlers/   # Each file/directory type gets its own handler module
│           ├── mod.rs
│           ├── root.rs         # Logic for the root directory (/)
│           ├── models.rs       # Logic for /models and its children
│           ├── conversations.rs  # Logic for /conversations and its children (prompt, history, etc.)
│           ├── config.rs       # Logic for /config and its children
│           └── semantic_search.rs # Logic for /semantic_search and its children
│
└── tests/
    ├── integration_tests.rs # High-level tests that interact with a mock FS
    └── unit_tests/
        └── config_parsing.rs # Example unit test

```



### **5. 总结**

这个设计方案将 LLM 的强大功能无缝地集成到了类 Unix 环境的核心工作流中。用户不再需要特殊的客户端或 Web UI，而是可以使用他们已经熟悉的、功能强大的 shell 工具链与 AI 进行深度、可编程的交互。从简单的问答到复杂的语义搜索，再到让 LLM 自我管理文件系统，所有操作都统一在“读写文件”这一简单而普适的模型之下，完美诠释了“一切皆文件”的设计哲学。







