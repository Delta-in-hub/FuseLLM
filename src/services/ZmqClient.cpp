// src/services/ZmqClient.cpp
#include "ZmqClient.h"

#include <zmq.h>

namespace fusellm {

ZmqClient::ZmqClient() {
    context_ = zmq_ctx_new();
    socket_ = zmq_socket(context_, ZMQ_REQ);
}

ZmqClient::~ZmqClient() {
    if (socket_) {
        zmq_close(socket_);
    }
    if (context_) {
        zmq_ctx_destroy(context_);
    }
}

// 与 Python 语义搜索服务通过 ZMQ 通信的实现将在这里完成

} // namespace fusellm
