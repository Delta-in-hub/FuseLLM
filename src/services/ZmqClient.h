// src/services/ZmqClient.h
#pragma once

#include "../common/data.h"
#include <string>
#include <vector>

namespace fusellm {

// 通过 ZeroMQ 与 Python 语义搜索服务通信
class ZmqClient {
public:
    ZmqClient();
    ~ZmqClient();

    // 连接到 ZMQ 服务端
    void connect(const std::string& endpoint);

    // 向Python服务发送请求并接收结果
    // op: 'create_index', 'delete_index', 'add_document', 'search'
    // payload: JSON 格式的请求数据
    std::string send_request(const std::string& op, const std::string& payload);

private:
    // ZMQ context 和 socket 的实现细节
    void* context_ = nullptr;
    void* socket_ = nullptr;
};

} // namespace fusellm
