
![FuseLLM Concept Diagram](assets/image-20250710220325924.png)

# **FuseLLM**
> https://github.com/Delta-in-hub/FuseLLM

**Mount your LLM.**

*Everything is a file. Even the LLM.*

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/actions)
[![License](https://img.shields.io/badge/license-MPL-blue)](LICENSE)
[![Language](https://img.shields.io/badge/language-C++17-purple.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://www.linux.org/)

**FuseLLM** is a filesystem based on FUSE (Filesystem in Userspace) that ingeniously maps interactions with Large Language Models (LLMs) to standard file and directory operations. By mounting an LLM into your system, you can use any standard command-line tool (`ls`, `cat`, `echo`, `grep`) to interact with AI seamlessly and programmatically.

The design philosophy is simple: **Everything is a file**. 
We unify complex AI interactions under the universal model of "reading and writing files," integrating LLM capabilities into the classic Unix workflow.


---

### Core Features

*   **Everything is a File**: Abstracts session management, queries, configuration, context, and even semantic search into file and directory operations.
*   **Native Shell Toolchain**: Use the tools you already know and love—`cat`, `echo`, `mkdir`, `rm`—to interact with the LLM. No special clients needed.
*   **Stateless & Stateful Interactions**:
    *   Perform quick, one-off "out-of-the-box" Q&A in the `/models` directory.
    *   Create persistent conversations with independent context and history in the `/conversations` directory.
*   **Dynamic Configuration**: View and adjust global, model-specific, or session-specific parameters in real-time by reading and writing to virtual TOML configuration files.
*   **Integrated Semantic Search**: Built-in vector-based semantic search. Simply drop documents into a `corpus` directory to build an index and write to a `query` file to search.
*   **Highly Extensible**: A clean, modular design (Handler pattern) makes it easy to add new features and top-level directories.
*   **Modern Cpp**: The core of FuseLLM is built using modern C++17 features, emphasizing safety, expressiveness, and zero-cost abstractions.
*   **CI/CD**: Project includes Github Action CI/CD pipelines with unit testing and integration testing
    *   **Sanitizers**: The project is built with sanitizers for runtime bug detection.

---

### Quick Start

Here is a simple demonstration of how to interact with FuseLLM from the command line:

```bash
# Mount FuseLLM (see Installation and Setup for details)
$ ./build/fusellm -m /tmp/llm -c .settings.toml

# 1. List all available models
$ ls -l /tmp/llm/models
-rw-rw-rw- 1 user user 4096 Jul 13 04:30 default
-rw-rw-rw- 1 user user 4096 Jul 13 04:30 gpt-4
-rw-rw-rw- 1 user user 4096 Jul 13 04:30 deepseek-v3

# 2. Ask a quick, stateless question
$ echo "Write a Hello World program in C++" > /tmp/llm/models/gpt-4

# 3. Read the model's response
$ cat /tmp/llm/models/gpt-4
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}

# 4. Create a stateful, persistent conversation
$ mkdir /tmp/llm/conversations/my-project-chat

# 5. Ask a question in the session; the AI will remember the context
$ echo "My project uses C++17" > /tmp/llm/conversations/my-project-chat/prompt
$ cat /tmp/llm/conversations/my-project-chat/prompt
Noted. Your project uses C++17. How can I help you?

$ echo "Please explain what structured bindings are" > /tmp/llm/conversations/my-project-chat/prompt
$ cat /tmp/llm/conversations/my-project-chat/prompt
Of course. In C++17, structured bindings allow you to decompose an object (like a struct, pair, or tuple) into its individual members with a single declaration...

# 6. View the complete conversation history
$ cat /tmp/llm/conversations/my-project-chat/history
[SYSTEM]
You are a helpful assistant.

[USER]
My project uses C++17

[AI]
Noted. Your project uses C++17. How can I help you?

[USER]
Please explain what structured bindings are

[AI]
Of course. In C++17, structured bindings allow you to decompose an object (like a struct, pair, or tuple) into its individual members with a single declaration...
```

---

### Installation and Setup

Follow these steps to install and configure FuseLLM.

#### 1. Prerequisites and Dependencies

We provide a convenient script to install all necessary system dependencies on Debian-based systems (like Ubuntu).

```bash
# Run from the project root directory
sudo bash ./scripts/debian.sh
```

This script will automatically install core dependencies like `cmake`, `g++`, `libfuse3-dev`, `libzmq3-dev`, and `ninja-build`.

#### 2. Clone and Build

```bash
# Clone the repository
git clone https://github.com/Delta-in-hub/FuseLLM
cd fusellm

# Initialize and pull all submodules (e.g., openai-cpp, fusepp)
git submodule update --init --recursive

# Run the build script
./build.sh
# On success, the executable will be located at build/fusellm
```

#### 3. Configure the Python Semantic Search Service

The semantic search feature is powered by a separate Python service.

```bash
# Navigate to the service directory
cd semantic_search_service

# Install Python dependencies
pip install -r requirements.txt

# Start the service (keep this terminal open)
# Note: The endpoint address here must match the one in your config file
python service.py --endpoint "ipc:///tmp/fusellm-semantic.ipc"
```

#### 4. Create the FuseLLM Configuration File

Create a file named `.settings.toml` in the project root directory and fill it with your API information.

```toml
# .settings.toml

# Required: Your LLM service API Key
api_key = "sk-YOUR_API_KEY_HERE"

# Optional: If you use a proxy or a local LLM, specify its OpenAI-compatible API base URL
base_url = "https://api.openai.com/v1/" # Example

# [semantic_search] table
[semantic_search]
# Required: Ensure this address exactly matches the one used to start the Python service
service_url = "ipc:///tmp/fusellm-semantic.ipc"

# [default_config] table (Optional)
[default_config]
# Set global default parameters here
temperature = 0.7
system_prompt = "You are a helpful programming assistant."
```

#### 5. Mount FuseLLM

Once everything is ready, open a new terminal to mount the filesystem.

```bash
# Ensure the mount point directory exists
mkdir -p /tmp/llm

# Run FuseLLM (keep this terminal in the foreground to see logs)
./build/fusellm -m /tmp/llm -c .settings.toml
```

You can now open a third terminal and start interacting with your LLM through the `/tmp/llm` directory!

---

### Filesystem Structure Explained

After mounting, the root directory contains four main directories:

*   `/models`: For stateless, one-off, quick Q&A.
    *   `ls -l`: Lists all available model files.
    *   `echo "Question" > <model_name>`: Sends a query to the specified model.
    *   `cat <model_name>`: Reads the last response from that model.

*   `/conversations`: For stateful, persistent, multi-turn dialogues.
    *   `mkdir <session_name>`: Creates a new conversation.
    *   `rmdir <session_name>`: Deletes a conversation and all its history.
    *   `/latest`: A symbolic link that always points to the most recently used session, greatly simplifying workflows.
    *   `.../<session_name>/prompt`: The core interaction file. Writing to it triggers a query; reading from it gets the response.
    *   `.../<session_name>/history`: (Read-only) Contains the full conversation history.
    *   `.../<session_name>/context`: (Read/Write) Provides temporary background information for the current session that is not part of the permanent history.
    *   `.../<session_name>/config/`: A directory for session-specific configuration, which has the highest priority.

*   `/config`: Manages global and model-specific configurations.
    *   `.../<model_name>/settings.toml`: (Read/Write) View or update parameters (like `temperature`) for a specific model.

*   `/semantic_search`: Provides vector-based semantic search capabilities.
    *   `mkdir <index_name>`: Creates a new search index.
    *   `.../<index_name>/corpus/`: The document corpus. Copying or writing files here will trigger indexing.
    *   `.../<index_name>/query`: The query interface. Write a question here, then read the file to get the most relevant document snippets.

---

### Development & Testing

*   **Build**: Use `./build.sh` (defaults to Debug mode) or `./build.sh release`.
*   **Debug**: The project is pre-configured with VSCode's `launch.json` and `tasks.json`. Simply press `F5` in VSCode to launch a debugging session.
*   **Unit Tests**: The project uses Doctest for unit testing. After building, the test executable is located at `build/test/fusellm_tests`. Configure the `OPENAI_API_KEY` and `OPENAI_API_BASE` in your environment variables.
*   **Integration Tests**: Run the `test/test.sh` script to perform a series of automated functional tests on the mounted filesystem. See `test/test.md` for more details. Configure the `OPENAI_API_KEY` and `OPENAI_API_BASE` in your environment variables.
*   **CI/CD**: Preconfigured Github Action CI/CD pipelines with unit and integration testing

---

### TODO

- [ ] Handle concurrent requests ([bshoshany/thread-pool](https://github.com/bshoshany/thread-pool)) and future.
- [ ] Improve the blocking behavior of `cat`

---

### License

This project is licensed under the [MPL 2.0 License](LICENSE).

> MPL is a compromise that keeps the modified source code open while allowing closed-source components to exist within the overall project.
