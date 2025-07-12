#include "ZmqClient.h"
#include "spdlog/spdlog.h"
#include <nlohmann/json.hpp>

// 包含 zmq_addon.hpp 以使用高级封装，如 zmq::multipart_t。
#include <zmq_addon.hpp>

namespace fusellm {

// 为通信设定一个超时时间（毫秒），以防止客户端在服务端无响应时永久阻塞。
constexpr int ZMQ_REQUEST_TIMEOUT_MS = 5000;

ZmqClient::ZmqClient()
    // 初始化 ZeroMQ 上下文，使用8个I/O线程。
    : context_(8),
      // 使用上下文创建一个 REQ (请求) 类型的套接字。
      socket_(context_, zmq::socket_type::req) {
    // 设置套接字的接收超时选项。
    socket_.set(zmq::sockopt::rcvtimeo, ZMQ_REQUEST_TIMEOUT_MS);
    SPDLOG_DEBUG("ZmqClient initialized.");
}

void ZmqClient::connect(const std::string &endpoint) {
    // 使用锁确保连接操作的线程安全。
    std::lock_guard<std::mutex> lock(mtx_);

    if (is_connected_) {
        SPDLOG_WARN("ZmqClient is already connected.");
        return;
    }

    try {
        socket_.connect(endpoint);
        is_connected_ = true;
        SPDLOG_INFO("ZmqClient successfully connected to endpoint: {}",
                     endpoint);
    } catch (const zmq::error_t &e) {
        // 如果连接失败，记录详细的错误信息。
        SPDLOG_ERROR(
            "ZmqClient failed to connect to endpoint '{}': {} (errno: {})",
            endpoint, e.what(), e.num());
        // 抛出异常或设置错误状态，让调用者知道失败。
        // 在这里我们仅记录错误，send_request 会检查 is_connected_ 状态。
    }
}

std::string ZmqClient::send_request(const std::string &op,
                                    const std::string &payload) {
    // 锁定互斥锁，以确保一次只有一个线程可以使用 REQ 套接字。
    // lock_guard 会在作用域结束时自动释放锁。
    std::lock_guard<std::mutex> lock(mtx_);

    if (!is_connected_) {
        SPDLOG_ERROR("Cannot send request: ZmqClient is not connected.");
        return R"({"error":"Client is not connected to the backend service."})";
    }

    try {
        // 1. 构造一个多部分消息 (multipart message) 用于发送。
        zmq::multipart_t request_msg;
        request_msg.addstr(op);      // 第一部分：操作码
        request_msg.addstr(payload); // 第二部分：载荷

        SPDLOG_DEBUG("Sending ZMQ request. Op: '{}', Payload size: {}", op,
                      payload.length());

        // 2. 发送请求。send() 方法会处理多部分消息的发送。
        request_msg.send(socket_);

        // 3. 等待并接收回复。
        zmq::multipart_t reply_msg;
        // recv() 会在超时或成功接收后返回。返回值为 true 表示成功。
        if (reply_msg.recv(socket_)) {
            // 假设回复总是一个单部分消息，其中包含结果字符串。
            if (reply_msg.size() == 1) {
                std::string reply_str = reply_msg.popstr();
                SPDLOG_DEBUG("Received ZMQ reply for op '{}'. Reply size: {}",
                              op, reply_str.length());
                return reply_str;
            } else {
                SPDLOG_WARN("Received unexpected multipart reply ({} parts) "
                             "for op '{}'. Discarding.",
                             reply_msg.size(), op);
                return R"({"error":"Received malformed multipart reply from service."})";
            }
        } else {
            // recv() 返回 false 表示操作超时。
            SPDLOG_WARN("ZMQ request for op '{}' timed out after {} ms.", op,
                         ZMQ_REQUEST_TIMEOUT_MS);
            return R"({"error":"Request to backend service timed out."})";
        }
    } catch (const zmq::error_t &e) {
        // 捕获任何其他 ZMQ 相关的运行时错误。
        SPDLOG_ERROR("ZMQ communication failed during request for op '{}': {} "
                      "(errno: {})",
                      op, e.what(), e.num());
        return R"({"error":"A ZMQ communication error occurred."})";
    }
}

} // namespace fusellm