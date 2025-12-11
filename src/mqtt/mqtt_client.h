#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/mqtt5/mqtt_client.hpp>

#include <functional>
#include <string>
#include <thread>

/**
 * @struct MqttConfig
 * @brief MQTT 配置结构体，包含连接所需的信息
 */
struct MqttConfig {
    std::string host;
    uint16_t port;
    std::string client_id;
};

/**
 * @class MqttSubscriber
 * @brief MQTT 订阅客户端，用于连接 MQTT 代理并接收消息
 */
class MqttSubscriber {
  public:
    using client_type = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>;
    using MessageCallback = std::function<void(const std::string &topic, const std::string &payload)>;

    static MqttConfig loadMqttConfig(const std::string &filename);

    static std::string generate_client_id(const std::string &app_name = "Lab2QRCode");

    MqttSubscriber(const std::string &host,
                   uint16_t port,
                   const std::string &client_id,
                   const MessageCallback &callback);
    void subscribe(const std::string &topic);
    void stop();
    ~MqttSubscriber();

  private:
    void start_receive();

    std::unique_ptr<boost::asio::io_context> ioc_;
    client_type client_;
    std::string host_;
    uint16_t port_;
    std::string client_id_;
    std::string topic_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    std::thread runner_thread_;
    MessageCallback callback_;
};
