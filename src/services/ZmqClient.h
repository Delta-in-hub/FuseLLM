#pragma once

#include <mutex>
#include <string>

// cppzmq 是一个头文件only的库，直接包含即可。
// 它为 libzmq C API 提供了 RAII 封装和异常处理。
#include <zmq.hpp>

namespace fusellm {

/**
 * @class ZmqClient
 * @brief 通过 ZeroMQ (REQ-REP模式) 与 Python 后端服务通信的客户端。
 *
 * 此类封装了与 Python 语义搜索服务交互的细节。
 * 它是线程安全的，允许多个 FUSE 回调线程通过它发送请求。
 */
class ZmqClient {
  public:
    /**
     * @brief 构造一个新的 ZmqClient 实例。
     *
     * 初始化 ZeroMQ 上下文和 REQ 类型的套接字。
     * 还为套接字设置了一个合理的接收超时时间，以防止无限期阻塞。
     */
    ZmqClient();

    /**
     * @brief 默认析构函数。
     *
     * 由于使用了 RAII 类型的 zmq::socket_t 和 zmq::context_t，
     * 资源会自动被正确释放。
     */
    ~ZmqClient() = default;

    // 删除拷贝构造和赋值操作，确保单例或受控的实例生命周期。
    ZmqClient(const ZmqClient &) = delete;
    ZmqClient &operator=(const ZmqClient &) = delete;

    /**
     * @brief 连接到 ZeroMQ 服务端点。
     * @param endpoint ZeroMQ 端点地址 (例如 "ipc:///tmp/fusellm.ipc" 或
     * "tcp://localhost:5555")。
     *
     * 此方法是线程安全的。
     */
    void connect(const std::string &endpoint);

    /**
     * @brief 向 Python 服务发送一个两部分的请求并等待回复。
     *
     * 请求是一个多部分消息：
     * - Part 1: 操作码 (op), e.g., "create_index", "add_document", "query"。
     * - Part 2: 载荷 (payload), 通常是一个 JSON 字符串，包含操作所需的数据。
     *
     * 此方法是线程安全的，内部使用互斥锁来序列化对单个 REQ 套接字的访问。
     *
     * @param op 要执行的操作的字符串标识符。
     * @param payload 与操作相关的数据，通常为 JSON 格式。
     * @return 来自 Python 服务的字符串响应，通常也是 JSON 格式。
     *         如果发生错误（如超时或通信失败），将返回一个包含 "error" 键的
     * JSON 字符串。
     */
    std::string send_request(const std::string &op, const std::string &payload);

  private:
    // ZeroMQ 上下文，是所有套接字的基础。
    zmq::context_t context_;

    // ZeroMQ REQ (请求) 类型套接字。
    zmq::socket_t socket_;

    // 用于保护对 socket_ 的并发访问的互斥锁，因为 REQ 套接字是有状态的，
    // 不能被多个线程同时使用。
    std::mutex mtx_;

    // 标记连接状态，以避免重复连接。
    bool is_connected_ = false;
};

} // namespace fusellm