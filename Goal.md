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



#### API 设计


```
OK, here is the complete, unabridged API design for every file as requested.

---

### `src/error.rs`

```rust
use std::io;
use thiserror::Error;

/// A unified Result type for the FuseLLM application.
pub type Result<T> = std::result::Result<T, FuseLlmError>;

/// The primary error enum for all possible failures in FuseLLM.
#[derive(Debug, Error)]
pub enum FuseLlmError {
    #[error("I/O error: {0}")]
    Io(#[from] io::Error),

    #[error("TOML deserialization error: {0}")]
    TomlDe(#[from] toml::de::Error),

    #[error("TOML serialization error: {0}")]
    TomlSer(#[from] toml::ser::Error),

    #[error("LLM API error: {0}")]
    LlmApi(#[from] async_openai::error::OpenAIError),
    
    #[error("Semantic search service communication error: {0}")]
    SemanticSearch(String),

    #[error("ZMQ communication error: {0}")]
    Zmq(#[from] zmq::Error),
    
    #[error("JSON serialization/deserialization error: {0}")]
    Json(#[from] serde_json::Error),

    #[error("Invalid path or entry not found")]
    NotFound,

    #[error("Permission denied")]
    PermissionDenied,

    #[error("Operation is not supported for this entry")]
    NotSupported,

    #[error("Filesystem entry already exists")]
    AlreadyExists,

    #[error("Directory is not empty")]
    NotEmpty,

    #[error("Invalid argument provided: {0}")]
    InvalidArgument(String),
    
    #[error("Operation requires a file, but entry is a directory")]
    IsDirectory,

    #[error("Operation requires a directory, but entry is a file")]
    NotDirectory,

    #[error("Entry is read-only")]
    ReadOnly,
    
    #[error("Internal state is locked or poisoned")]
    StateLock,
}

impl From<FuseLlmError> for i32 {
    /// Converts the custom error into a standard C integer error code for FUSE.
    fn from(err: FuseLlmError) -> Self {
        match err {
            FuseLlmError::NotFound => libc::ENOENT,
            FuseLlmError::PermissionDenied => libc::EPERM,
            FuseLlmError::Io(_) => libc::EIO,
            FuseLlmError::AlreadyExists => libc::EEXIST,
            FuseLlmError::NotEmpty => libc::ENOTEMPTY,
            FuseLlmError::InvalidArgument(_) => libc::EINVAL,
            FuseLlmError::IsDirectory => libc::EISDIR,
            FuseLlmError::NotDirectory => libc::ENOTDIR,
            FuseLlmError::ReadOnly => libc::EROFS,
            FuseLlmError::NotSupported => libc::ENOSYS,
            _ => libc::EIO, // Default to a generic I/O error for other cases
        }
    }
}
```

---

### `src/config.rs`

```rust
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::path::Path;
use crate::error::Result;

/// Settings applicable to any LLM model interaction.
/// All fields are optional to allow for easy merging and overriding.
#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct ModelSettings {
    pub temperature: Option<f32>,
    pub top_p: Option<f32>,
    pub max_tokens: Option<u16>,
}

/// The top-level configuration loaded from `settings.toml`.
#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct GlobalConfig {
    pub default_model: Option<String>,
    #[serde(flatten)]
    pub default_settings: ModelSettings,
    #[serde(default)]
    pub models: BTreeMap<String, ModelSettings>,
}

impl GlobalConfig {
    /// Loads the global configuration from a specified TOML file.
    pub fn load_from_path(path: &Path) -> Result<Self> {
        unimplemented!();
    }
}

/// Configuration specific to a single conversation session.
/// These settings have the highest precedence.
#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct SessionConfig {
    pub model: Option<String>,
    pub system_prompt: Option<String>,
    #[serde(flatten)]
    pub settings: ModelSettings,
}

impl SessionConfig {
    /// Merges this session's configuration with the global configuration
    /// to produce the final, effective settings for an LLM API call.
    /// It returns the model name to use and the final combined settings.
    pub fn get_effective_settings<'a>(
        &'a self, 
        global_config: &'a GlobalConfig
    ) -> (&'a str, ModelSettings) {
        unimplemented!();
    }
}
```

---

### `src/state.rs`

```rust
use crate::config::{GlobalConfig, SessionConfig, ModelSettings};
use std::collections::BTreeMap;
use std::sync::{Arc, RwLock};
use std::time::SystemTime;

pub type SharedState = Arc<RwLock<FilesystemState>>;

/// A message within a conversation's history.
#[derive(Debug, Clone)]
pub enum ChatMessage {
    System(String),
    User(String),
    Assistant(String),
}

/// Represents a single, stateful conversation.
#[derive(Debug)]
pub struct Conversation {
    pub id: String,
    pub config: SessionConfig,
    pub history: Vec<ChatMessage>,
    pub context: String,
    pub latest_response: String,
    pub created_at: SystemTime,
    pub last_modified: SystemTime,
}

impl Conversation {
    /// Creates a new conversation with a given ID, inheriting from global config.
    pub fn new(id: String, global_config: &GlobalConfig) -> Self {
        unimplemented!();
    }
}

/// Represents a semantic search index.
#[derive(Debug)]
pub struct SearchIndex {
    pub id: String,
    /// A list of file names that have been added to the corpus.
    pub corpus_files: Vec<String>,
    /// The last query's result, formatted for reading.
    pub latest_query_result: String,
    pub created_at: SystemTime,
}

impl SearchIndex {
    pub fn new(id: String) -> Self {
        unimplemented!();
    }
}

/// The stateless representation of a model for direct interaction.
#[derive(Debug)]
pub struct StatelessModel {
    pub id: String,
    pub latest_response: String,
}

/// The single source of truth for the filesystem's dynamic state.
#[derive(Debug)]
pub struct FilesystemState {
    pub config: GlobalConfig,
    pub conversations: BTreeMap<String, Conversation>,
    pub search_indexes: BTreeMap<String, SearchIndex>,
    pub stateless_models: BTreeMap<String, StatelessModel>,
    /// The session ID of the most recently used conversation.
    pub latest_session_id: Option<String>,
    /// A counter for dynamically assigning new inodes.
    next_inode: u64,
}

impl FilesystemState {
    /// Initializes the filesystem state with a global config.
    pub fn new(config: GlobalConfig) -> Self {
        unimplemented!();
    }
    
    /// Atomically retrieves the next available inode number.
    pub fn allocate_inode(&mut self) -> u64 {
        unimplemented!();
    }

    /// Updates the `latest_session_id` and the session's last_modified time.
    pub fn mark_session_used(&mut self, session_id: &str) {
        unimplemented!();
    }
}
```

---

### `src/llm_api.rs`

```rust
use async_openai::{Client, types::{CreateChatCompletionRequestArgs, ChatCompletionRequestMessage}};
use crate::config::ModelSettings;
use crate::error::Result;
use crate::state::ChatMessage;

/// A client for interacting with a Large Language Model.
#[derive(Clone)]
pub struct LlmClient {
    client: Client,
}

impl LlmClient {
    /// Creates a new LLM client, typically configured from environment variables.
    pub fn new() -> Result<Self> {
        unimplemented!();
    }

    /// Sends a request to the LLM and returns the text of the assistant's response.
    /// This is an async function and will need to be handled by a runtime.
    pub async fn chat(
        &self,
        model: &str,
        messages: &[ChatMessage],
        settings: &ModelSettings,
    ) -> Result<String> {
        unimplemented!();
    }
}

/// Utility to convert internal ChatMessage enum to the one required by async-openai.
fn to_openai_messages(messages: &[ChatMessage]) -> Vec<ChatCompletionRequestMessage> {
    unimplemented!();
}
```

---

### `src/semantic/client.rs`

```rust
use crate::error::{Result, FuseLlmError};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug)]
pub enum Request {
    CreateIndex { index_id: String },
    DeleteIndex { index_id: String },
    AddDocument { index_id: String, file_name: String, content: String },
    RemoveDocument { index_id: String, file_name: String },
    Query { index_id: String, text: String },
    Ping,
}

#[derive(Serialize, Deserialize, Debug)]
pub enum Response {
    Success,
    Pong,
    QueryResult(String),
    Error(String),
}

/// A synchronous client for the ZMQ semantic search service.
pub struct SemanticClient {
    socket: zmq::Socket,
}

impl SemanticClient {
    /// Creates a new client and connects to the ZMQ endpoint.
    pub fn new(endpoint: &str) -> Result<Self> {
        unimplemented!();
    }

    /// Sends a request and blocks until a response is received.
    fn send_request(&self, request: &Request) -> Result<Response> {
        unimplemented!();
    }

    pub fn create_index(&self, index_id: &str) -> Result<()> { unimplemented!(); }
    pub fn delete_index(&self, index_id: &str) -> Result<()> { unimplemented!(); }
    pub fn add_document(&self, index_id: &str, file_name: &str, content: &str) -> Result<()> { unimplemented!(); }
    pub fn remove_document(&self, index_id: &str, file_name: &str) -> Result<()> { unimplemented!(); }
    pub fn query(&self, index_id: &str, text: &str) -> Result<String> { unimplemented!(); }
    
    /// Checks if the Python service is alive and responding.
    pub fn ping(&self) -> Result<()> { unimplemented!(); }
}
```

---

### `src/fs/constants.rs`

```rust
pub const ROOT_DIR_INO: u64 = 1;
pub const CONFIG_DIR_INO: u64 = 2;
pub const MODELS_DIR_INO: u64 = 3;
pub const CONVERSATIONS_DIR_INO: u64 = 4;
pub const SEMANTIC_SEARCH_DIR_INO: u64 = 5;

// File inodes within CONFIG_DIR
pub const GLOBAL_SETTINGS_INO: u64 = 10;
pub const CONFIG_MODELS_DIR_INO: u64 = 11;

// Symlinks
pub const MODELS_DEFAULT_SYMLINK_INO: u64 = 50;
pub const CONVERSATIONS_LATEST_SYMLINK_INO: u64 = 51;

// The start of the dynamic inode range.
// Any inode number >= DYNAMIC_INODE_START is allocated at runtime.
pub const DYNAMIC_INODE_START: u64 = 1_000_000;
```

---

### `src/fs/mod.rs`

```rust
use fuser::{
    Filesystem, MountOption, ReplyAttr, ReplyData, ReplyDirectory, ReplyEmpty,
    ReplyEntry, ReplyLseek, ReplyOpen, ReplyWrite, ReplyXattr, Request,
    FUSE_ROOT_ID,
};
use std::ffi::OsStr;

use crate::llm_api::LlmClient;
use crate::semantic::client::SemanticClient;
use crate::state::SharedState;

pub mod constants;
pub mod handlers;

/// The main struct representing our FUSE filesystem.
pub struct FuseLlm {
    pub state: SharedState,
    pub llm_client: LlmClient,
    pub semantic_client: SemanticClient,
    /// An async runtime to execute async tasks (like LLM calls) from sync FUSE calls.
    pub runtime: async_std::runtime::Runtime, 
}

impl Filesystem for FuseLlm {
    fn lookup(&mut self, _req: &Request, parent: u64, name: &OsStr, reply: ReplyEntry) {
        unimplemented!(); 
    }

    fn getattr(&mut self, _req: &Request, ino: u64, reply: ReplyAttr) {
        unimplemented!();
    }

    fn read(
        &mut self, _req: &Request, ino: u64, _fh: u64, offset: i64, 
        _size: u32, _flags: i32, _lock_owner: Option<u64>, reply: ReplyData
    ) {
        unimplemented!();
    }
    
    fn write(
        &mut self, _req: &Request, ino: u64, _fh: u64, _offset: i64,
        data: &[u8], _write_flags: u32, _flags: i32, _lock_owner: Option<u64>,
        reply: ReplyWrite,
    ) {
        unimplemented!();
    }

    fn readdir(
        &mut self, _req: &Request, ino: u64, _fh: u64, offset: i64,
        reply: ReplyDirectory,
    ) {
        unimplemented!();
    }
    
    fn mkdir(
        &mut self, _req: &Request, parent: u64, name: &OsStr,
        _mode: u32, _umask: u32, reply: ReplyEntry
    ) {
        unimplemented!();
    }

    fn rmdir(&mut self, _req: &Request, parent: u64, name: &OsStr, reply: ReplyEmpty) {
        unimplemented!();
    }

    fn readlink(&mut self, _req: &Request, ino: u64, reply: ReplyData) {
        unimplemented!();
    }

    fn setattr(
        &mut self, _req: &Request, ino: u64, _mode: Option<u32>, _uid: Option<u32>,
        _gid: Option<u32>, size: Option<u64>, _atime: Option<fuser::TimeOrNow>,
        _mtime: Option<fuser::TimeOrNow>, _ctime: Option<std::time::SystemTime>,
        _fh: Option<u64>, _crtime: Option<std::time::SystemTime>,
        _chgtime: Option<std::time::SystemTime>, _bkuptime: Option<std::time::SystemTime>,
        _flags: Option<u32>, reply: ReplyAttr
    ) {
        // Primarily used for `truncate` behavior when writing to files.
        unimplemented!();
    }

    // ... Other trait methods like open, release, create, unlink etc. will be stubs or simple pass-throughs
    // as our filesystem logic is mostly contained in read/write/mkdir/rmdir.
}
```

---

### `src/fs/handlers/*.rs`

#### **`src/fs/handlers/root.rs`**
```rust
use fuser::{ReplyDirectory, FileType};
use crate::fs::constants::*;
use crate::FuseLlm;
use crate::error::Result;

/// Handles `ls /`
pub fn readdir(fs: &FuseLlm, reply: &mut ReplyDirectory) -> Result<()> {
    // Add ., .., config, models, conversations, semantic_search
    unimplemented!();
}
```

#### **`src/fs/handlers/config.rs`**
```rust
use fuser::{ReplyAttr, ReplyData, ReplyDirectory, ReplyWrite};
use crate::FuseLlm;
use crate::error::Result;

/// Handles `ls /config` and `ls /config/models/[name]`
pub fn readdir(fs: &FuseLlm, ino: u64, reply: &mut ReplyDirectory) -> Result<()> {
    unimplemented!();
}

/// Handles `cat /config/settings.toml` and other config files
pub fn read_config_file(fs: &FuseLlm, ino: u64, reply: ReplyData) -> Result<()> {
    unimplemented!();
}

/// Handles `echo "..." > /config/settings.toml`
pub fn write_config_file(fs: &mut FuseLlm, ino: u64, data: &[u8], reply: ReplyWrite) -> Result<()> {
    unimplemented!();
}
```

#### **`src/fs/handlers/models.rs`**
```rust
use fuser::{ReplyData, ReplyDirectory, ReplyWrite};
use crate::FuseLlm;
use crate::error::Result;

/// Handles `ls /models`
pub fn readdir(fs: &FuseLlm, reply: &mut ReplyDirectory) -> Result<()> {
    unimplemented!();
}

/// Handles `cat /models/[name]`
pub fn read_model_response(fs: &FuseLlm, model_name: &str, reply: ReplyData) -> Result<()> {
    unimplemented!();
}

/// Handles `echo "..." > /models/[name]`
pub fn write_to_model(fs: &mut FuseLlm, model_name: &str, data: &[u8], reply: ReplyWrite) -> Result<()> {
    unimplemented!();
}
```

#### **`src/fs/handlers/conversations.rs`**
```rust
use fuser::{ReplyData, ReplyDirectory, ReplyEmpty, ReplyEntry, ReplyWrite};
use crate::FuseLlm;
use crate::error::Result;

/// Handles `ls /conversations`
pub fn readdir(fs: &FuseLlm, reply: &mut ReplyDirectory) -> Result<()> {
    unimplemented!();
}

/// Handles `mkdir /conversations/my-chat`
pub fn mkdir(fs: &mut FuseLlm, name: &str, reply: ReplyEntry) -> Result<()> {
    unimplemented!();
}

/// Handles `rmdir /conversations/my-chat`
pub fn rmdir(fs: &mut FuseLlm, name: &str, reply: ReplyEmpty) -> Result<()> {
    unimplemented!();
}

/// Handles `cat /conversations/[id]/prompt`
pub fn read_prompt(fs: &FuseLlm, session_id: &str, reply: ReplyData) -> Result<()> {
    unimplemented!();
}

/// Handles `echo "..." > /conversations/[id]/prompt`
pub fn write_prompt(fs: &mut FuseLlm, session_id: &str, data: &[u8], reply: ReplyWrite) -> Result<()> {
    unimplemented!();
}

/// Handles `cat /conversations/[id]/history`
pub fn read_history(fs: &FuseLlm, session_id: &str, reply: ReplyData) -> Result<()> {
    unimplemented!();
}

/// Handles `cat /conversations/[id]/context`
pub fn read_context(fs: &FuseLlm, session_id: &str, reply: ReplyData) -> Result<()> {
    unimplemented!();
}

/// Handles `echo "..." > /conversations/[id]/context`
pub fn write_context(fs: &mut FuseLlm, session_id: &str, data: &[u8], reply: ReplyWrite) -> Result<()> {
    unimplemented!();
}

// ... other handlers for the session's 'config' subdirectory
```

#### **`src/fs/handlers/semantic_search.rs`**
```rust
use fuser::{ReplyData, ReplyDirectory, ReplyEmpty, ReplyEntry, ReplyWrite};
use crate::FuseLlm;
use crate::error::Result;

/// Handles `ls /semantic_search`
pub fn readdir(fs: &FuseLlm, reply: &mut ReplyDirectory) -> Result<()> {
    unimplemented!();
}

/// Handles `mkdir /semantic_search/my-index`
pub fn mkdir(fs: &mut FuseLlm, name: &str, reply: ReplyEntry) -> Result<()> {
    unimplemented!();
}

/// Handles `rmdir /semantic_search/my-index`
pub fn rmdir(fs: &mut FuseLlm, name: &str, reply: ReplyEmpty) -> Result<()> {
    unimplemented!();
}

/// Handles `echo "..." > /semantic_search/[id]/query`
pub fn write_query(fs: &mut FuseLlm, index_id: &str, data: &[u8], reply: ReplyWrite) -> Result<()> {
    unimplemented!();
}

/// Handles `cat /semantic_search/[id]/query`
pub fn read_query_result(fs: &FuseLlm, index_id: &str, reply: ReplyData) -> Result<()> {
    unimplemented!();
}

/// Logic for when a file is created/written in the `corpus` directory.
/// Note: This is triggered by `create` or `write` syscalls on a file inside the corpus dir.
pub fn write_to_corpus(fs: &mut FuseLlm, index_id: &str, file_name: &str, data: &[u8]) -> Result<()> {
    unimplemented!();
}

/// Logic for when a file is deleted from the `corpus` directory.
/// Note: This is triggered by an `unlink` syscall.
pub fn remove_from_corpus(fs: &mut FuseLlm, index_id: &str, file_name: &str) -> Result<()> {
    unimplemented!();
}
```

---

### `src/main.rs`

```rust
use clap::Parser;
use fuser::MountOption;
use std::env;
use std::path::PathBuf;
use std::sync::{Arc, RwLock};

mod config;
mod error;
mod fs;
// The lib.rs file is often omitted in a binary crate if all modules are declared here.
// mod lib; 
mod llm_api;
mod semantic;
mod state;

use config::GlobalConfig;
use error::Result;
use fs::FuseLlm;
use llm_api::LlmClient;
use semantic::client::SemanticClient;
use state::{FilesystemState, SharedState};


#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Path to the mountpoint for the FUSE filesystem
    #[arg(required = true)]
    mount_point: PathBuf,

    /// Path to the global settings TOML file
    #[arg(short, long, default_value = "settings.toml")]
    config: PathBuf,

    /// ZMQ endpoint for the semantic search service
    #[arg(short, long, default_value = "ipc:///tmp/semantic.ipc")]
    semantic_endpoint: String,
}

fn main() -> Result<()> {
    env_logger::init();
    let args = Args::parse();

    log::info!("Starting FuseLLM...");
    log::info!("Mounting at: {}", args.mount_point.display());
    log::info!("Loading config from: {}", args.config.display());

    let global_config = GlobalConfig::load_from_path(&args.config)?;
    let state = FilesystemState::new(global_config);
    let shared_state: SharedState = Arc::new(RwLock::new(state));

    let llm_client = LlmClient::new()?;
    let semantic_client = SemanticClient::new(&args.semantic_endpoint)?;
    
    // Check if the semantic service is running
    if let Err(e) = semantic_client.ping() {
        log::error!("Could not connect to semantic search service at {}: {}", args.semantic_endpoint, e);
        log::error!("Please ensure the Python service is running.");
        return Err(e);
    }
    log::info!("Semantic search service connected successfully.");

    let filesystem = FuseLlm {
        state: shared_state,
        llm_client,
        semantic_client,
        runtime: async_std::runtime::new().unwrap(),
    };

    let options = vec![
        MountOption::AutoUnmount,
        MountOption::FSName("fusellm".to_string()),
        MountOption::DefaultPermissions,
        MountOption::AllowOther, // Requires user_allow_other in /etc/fuse.conf
    ];

    fuser::mount2(filesystem, &args.mount_point, &options)?;

    Ok(())
}
```

```


### **5. 总结**

这个设计方案将 LLM 的强大功能无缝地集成到了类 Unix 环境的核心工作流中。用户不再需要特殊的客户端或 Web UI，而是可以使用他们已经熟悉的、功能强大的 shell 工具链与 AI 进行深度、可编程的交互。从简单的问答到复杂的语义搜索，再到让 LLM 自我管理文件系统，所有操作都统一在“读写文件”这一简单而普适的模型之下，完美诠释了“一切皆文件”的设计哲学。







