# FuseLLM 设计模式文档

- 单例模式确保了全局唯一的文件系统实例
- 模板方法模式和策略模式实现了灵活的请求处理机制
- CRTP 提供了高效的静态多态
- 组合模式促进了代码重用和低耦合
- 工厂方法模式简化了对象创建
- 外观模式提供了统一的系统接口
- 命令模式封装了服务请求


## 1. 单例模式 (Singleton Pattern)

### 说明
单例模式确保一个类只有一个实例，并提供一个全局访问点来访问该实例。这种模式在需要严格控制如何以及何时访问实例的情况下非常有用。

### 为什么使用它
- **全局唯一性**: FUSE 文件系统是整个应用程序的单一、核心资源。必须保证只有一个实例在管理挂载点、处理 FUSE 回调和协调所有内部状态（如会话、配置等），以防止数据不一致和资源冲突。
- **简化访问**: FUSE 的回调函数是静态的 C 风格函数。单例模式提供了一个便捷的 `getInstance()` 方法，使得这些静态回调函数可以轻松访问到文件系统的核心实例及其包含的成员（如 `SessionManager`、`LLMClient` 等）。

### 在项目中的应用
**FuseLLM 类**
- 实现位置：`src/fs/FuseLLM.h`
- 描述：FuseLLM 类是整个文件系统的核心，采用单例模式确保全局只有一个文件系统实例。
- 实现方式：
  - 私有构造函数防止外部直接创建实例
  - 删除拷贝构造函数和赋值操作符防止复制
  - 静态 `getInstance()` 方法提供全局访问点

```cpp
// src/fs/FuseLLM.h
class FuseLLM : public Fusepp::Fuse<FuseLLM> {
  public:
    // 获取单例的静态方法
    static FuseLLM &getInstance(ConfigManager &config);

    FuseLLM() = delete;
    FuseLLM(const FuseLLM &) = delete;
    FuseLLM(FuseLLM &&) = delete;
    FuseLLM &operator=(const FuseLLM &) = delete;
    FuseLLM &operator=(FuseLLM &&) = delete;

  private:
    explicit FuseLLM(ConfigManager &config);
    // ...
};
```

## 2. 模板方法模式 (Template Method Pattern)

### 说明
模板方法模式定义了一个算法的骨架，将一些步骤的实现延迟到子类。它允许子类在不改变算法结构的情况下重新定义算法的某些特定步骤。

### 为什么使用它
- **统一接口，简化扩展**: 项目的核心是“一切皆文件”，但不同路径下的文件行为迥异（例如，`/models` 下的文件代表一次性查询，而 `/conversations` 下的文件是持久化会话）。模板方法模式通过 `BaseHandler` 提供了一个统一的接口，所有的具体 Handler 都继承自它。这使得 `FuseLLM` 可以用同样的方式对待所有 Handler。
- **减少重复代码**: `BaseHandler` 为所有 FUSE 操作（`getattr`, `readdir` 等）提供了默认实现，即返回 `-ENOSYS`（功能未实现）。这意味着每个具体的 Handler 子类只需重写它所关心的操作，而无需为每个不支持的操作都编写样板代码，极大地简化了新 Handler 的开发。

### 在项目中的应用
**BaseHandler 类**
- 实现位置：`src/handlers/BaseHandler.h`
- 描述：BaseHandler 定义了所有处理器的公共接口和默认实现，允许子类只重写所需的方法。
- 实现方式：
  - 父类 BaseHandler 定义了文件系统操作的基本方法（如 getattr、readdir、open 等）
  - 子类（如 ConfigHandler、ConversationsHandler 等）根据需要重写特定方法

```cpp
// src/handlers/BaseHandler.h
class BaseHandler {
  public:
    virtual ~BaseHandler() = default;
    virtual int getattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi) {
        // 默认实现
        return -ENOSYS;
    }
    // 其他方法...
};
```

## 3. 策略模式 (Strategy Pattern)

### 说明
策略模式定义了一系列算法，使它们可以互相替换，让算法的变化独立于使用它的客户端。

### 为什么使用它
- **实现路径路由**: 这是实现“一切皆文件”哲学的关键。不同的文件路径（`/models`, `/config`, `/conversations`）需要完全不同的处理逻辑。策略模式将每一种处理逻辑（即一个 `Handler`）封装成一个独立的“策略”。`FuseLLM` 作为上下文，根据传入的 `path`，使用 `PathParser` 动态地选择并执行相应的策略。
- **高度可扩展性**: 该模式使得添加新的文件系统功能变得非常容易。例如，要增加一个新的顶层目录 `/new_feature`，只需创建一个新的 `NewFeatureHandler` 类，并在 `FuseLLM` 的路由表中注册它即可，无需修改任何现有的 Handler 或 `FuseLLM` 的核心路由逻辑。这完美支持了项目分层、模块化的目标。

### 在项目中的应用
**PathParser 与 Handler 机制**
- 实现位置：`src/fs/PathParser.h` 和 `src/fs/FuseLLM.h`
- 描述：根据请求路径类型（PathType）动态选择不同的处理器（Handler）。
- 实现方式：
  - PathParser 解析路径返回 PathType
  - FuseLLM 根据 PathType 选择合适的 Handler
  - 不同 Handler 实现相同接口但包含不同逻辑

```cpp
// src/fs/FuseLLM.h
static BaseHandler *get_handler(std::string_view path);

// 存储不同路径类型的处理器
static std::unordered_map<PathType, std::unique_ptr<BaseHandler>> handlers;
```

## 4. CRTP 模式 (Curiously Recurring Template Pattern)

### 说明
CRTP 是 C++ 的一种编译时多态技术，其中派生类将自身作为模板参数传递给基类模板。

### 为什么使用它
- **性能优化**: FUSE 的 C 接口是基于函数指针回调的。使用传统的虚函数（运行时多态）来实现回调会引入性能开销。`Fusepp` 库巧妙地使用 CRTP（编译时多态）来连接 FUSE 的 C API 和 C++ 的类方法。这允许基类模板直接调用派生类的静态方法，完全消除了虚函数调用的开销，对于文件系统这种性能敏感的应用至关重要。
- **类型安全**: CRTP 可以在编译期确保派生类实现了必要的方法，提供了比原始函数指针更强的类型安全。

### 在项目中的应用
**FuseLLM 类**
- 实现位置：`src/fs/FuseLLM.h`
- 描述：FuseLLM 通过 CRTP 继承自 Fusepp::Fuse<FuseLLM>。
- 实现方式：
  - 基类模板接收派生类类型作为模板参数
  - 基类可以静态调用派生类的方法，实现编译期多态
  - 避免了虚函数调用的运行时开销

```cpp
// src/fs/FuseLLM.h
class FuseLLM : public Fusepp::Fuse<FuseLLM> {
    // ...
};
```

## 5. 组合模式 (Composition Pattern)

### 说明
组合模式优先使用组合而非继承来实现类之间的关系，遵循"组合优于继承"的原则。

### 为什么使用它
- **灵活性和解耦**: 项目中的各个核心组件（`FuseLLM`, `SessionManager`, `LLMClient`, `ConfigManager`）职责分明。例如，`FuseLLM` *拥有*一个 `SessionManager`，而不是*是*一个 `SessionManager`。这种“has-a”关系（组合）比“is-a”关系（继承）更灵活。我们可以轻松地替换某个组件的实现（例如，用一个模拟的 `LLMClient` 进行测试）而无需改变其他组件，大大降低了模块间的耦合度。
- **清晰的职责划分**: 组合模式使得每个类的职责更加单一和明确，符合单一职责原则，使代码结构更清晰，更易于理解和维护。

### 在项目中的应用
**服务层和状态管理**
- 实现位置：`src/fs/FuseLLM.h`、`src/handlers/ConfigHandler.h` 等
- 描述：通过组合方式而非继承使用其他服务和组件。
- 实现方式：
  - FuseLLM 类组合了 SessionManager、LLMClient、ZmqClient
  - ConfigHandler 组合了 ConfigManager 和 LLMClient
  - 通过组合实现功能复用和依赖注入

```cpp
// src/fs/FuseLLM.h
class FuseLLM : public Fusepp::Fuse<FuseLLM> {
  private:
    // ...
    SessionManager session_manager;
    LLMClient llm_client;
    ZmqClient zmq_client;
    // ...
};
```

## 6. 工厂方法模式 (Factory Method Pattern)

### 说明
工厂方法模式定义一个用于创建对象的接口，但让子类决定实例化哪一个类。工厂方法使一个类的实例化延迟到其子类。

### 为什么使用它
- **封装复杂创建逻辑**: 创建一个 `Session` 对象不仅仅是 `new Session()`。它需要从 `ConfigManager` 获取默认配置，可能需要生成一个唯一的ID，并进行其他初始化。`SessionManager` 的 `create_session` 和 `create_session_with_auto_id` 方法作为工厂，将这些复杂的创建逻辑封装起来。
- **集中管理对象生命周期**: `SessionManager` 不仅负责创建 `Session`，还负责存储和销毁它们。工厂方法是实现这一点的自然选择，它确保了所有 `Session` 对象都由 `SessionManager` 统一管理，避免了对象所有权混乱和潜在的内存泄漏。

### 在项目中的应用
**SessionManager 类**
- 实现位置：`src/state/SessionManager.h`
- 描述：提供创建 Session 实例的方法。
- 实现方式：
  - `create_session()` 和 `create_session_with_auto_id()` 方法封装了 Session 对象的创建逻辑
  - 隐藏了对象创建的细节，提供统一的接口

```cpp
// src/state/SessionManager.h
class SessionManager {
  public:
    std::shared_ptr<Session> create_session(std::string_view id);
    std::shared_ptr<Session> create_session_with_auto_id();
    // ...
};
```

## 7. 外观模式 (Facade Pattern)

### 说明
外观模式为子系统中的一组接口提供了一个统一的高层接口，使子系统更容易使用。

### 为什么使用它
- **简化 FUSE 接口**: FUSE 内核模块不关心我们内部复杂的子系统（`Handlers`, `SessionManager`, `LLMClient` 等）。它只需要一个简单的、统一的入口点来处理文件系统操作。`FuseLLM` 类正扮演了这个外观角色。
- **隔离内部复杂性**: 它将所有来自 FUSE 的请求（`getattr`, `readdir` 等）路由到内部复杂的系统中进行处理，并将结果返回。这有效地将 FUSE C API 的底层细节与我们现代 C++ 的内部架构隔离开来，使得两边都可以独立演进。

### 在项目中的应用
**FuseLLM 类**
- 实现位置：`src/fs/FuseLLM.h`
- 描述：FuseLLM 作为整个文件系统的外观，向 FUSE 提供统一的接口。
- 实现方式：
  - 封装了内部复杂的子系统（如 SessionManager、Handlers 等）
  - 对外提供简单统一的 FUSE 操作接口（如 getattr、readdir、open 等）

```cpp
// src/fs/FuseLLM.h
class FuseLLM : public Fusepp::Fuse<FuseLLM> {
  public:
    // FUSE 回调函数，将作为 FUSE 操作的入口点
    static int getattr(const char *path, struct stat *stbuf,
                     struct fuse_file_info *fi);
    static int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags flags);
    // ...
};
```

## 8. 命令模式 (Command Pattern)

### 说明
命令模式将请求封装成对象，从而允许参数化不同的请求、队列或日志请求，以及支持可撤销的操作。

### 为什么使用它
- **封装请求细节**: 与 LLM 的交互是一个复杂的过程，需要构建特定的 JSON 请求体、处理认证、解析响应等。`LLMClient` 的方法（如 `simple_query`, `conversation_query`）将这些复杂的交互封装成了一个个独立的“命令”。
- **解耦调用者与接收者**: `ConversationsHandler` 只需调用 `llm_client.conversation_query()`，它不关心这个请求是如何被发送到 OpenAI、如何处理网络错误或如何解析 JSON 的。这种封装使得调用方（Handler）的逻辑更清晰，并且如果未来需要更换 LLM 服务提供商，只需修改 `LLMClient` 这个“命令”的内部实现，而无需触及任何调用它的代码。

### 在项目中的应用
**LLMClient 类**
- 实现位置：`src/services/LLMClient.h`
- 描述：封装了对 LLM 服务的请求操作。
- 实现方式：
  - 将不同类型的请求（simple_query、conversation_query）封装为方法
  - 每个方法构建适当的请求参数和上下文
  - 统一处理请求结果和错误情况

```cpp
// src/services/LLMClient.h
class LLMClient {
  public:
    // ...
    std::string simple_query(std::string_view model_name,
                           std::string_view prompt,
                           const ConfigManager &config_manager);
    
    std::string conversation_query(std::string_view model_name,
                                 const ConfigManager &config_manager,
                                 const Conversation &conversation);
    // ...
};
```
