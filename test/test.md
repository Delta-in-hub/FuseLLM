# FuseLLM 功能测试

本目录包含用于测试 FuseLLM 核心功能的自动化脚本。

## 准备工作

在运行测试脚本之前，请确保完成以下步骤：

1.  **编译项目**:
    确保您已经成功运行了项目根目录下的 `./build.sh` 脚本，并且 `build/fusellm` 可执行文件已经生成。

    ```bash
    ./build.sh
    ```

2.  **准备配置文件**:
    在项目根目录创建一个配置文件，例如 `.settings.toml`。此文件是必需的，因为它包含了连接 LLM API 的密钥。

    ```toml
    # .settings.toml
    api_key = "sk-YOUR_API_KEY"
    # 如果您使用本地或代理的 OpenAI 兼容 API，请设置 base_url
    base_url = "https://api.openai.com/v1/" 
    # 或者 base_url = "http://localhost:8080/v1/"

    [semantic_search]
    # 确保此地址与 Python 服务启动时使用的地址匹配
    service_url = "ipc:///tmp/fusellm-semantic.ipc"
    ```

3.  **启动 Python 语义搜索服务**:
    打开一个新的终端，进入 `semantic_search_service` 目录，安装依赖并启动服务。

    ```bash
    cd semantic_search_service
    pip install -r requirements.txt
    # 使用与 .settings.toml 中匹配的 endpoint 启动服务
    python service.py --endpoint "ipc:///tmp/fusellm-semantic.ipc"
    ```
    让此服务在后台持续运行。

4.  **挂载 FuseLLM**:
    打开另一个新的终端，在项目根目录启动并挂载 FuseLLM。

    ```bash
    # 确保挂载点存在
    mkdir -p /tmp/llm
    
    # 启动 FuseLLM
    ./build/fusellm -m /tmp/llm -c ./.settings.toml
    ```
    让 FuseLLM 进程在前台运行，以便观察其日志输出。

## 运行测试

一切准备就绪后，打开 **第三个终端**，进入 `test` 目录并执行测试脚本：

```bash
cd test
bash ./test.sh
```