#include "mqtt_client.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

MqttConfig MqttSubscriber::load_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        spdlog::error("Unable to open config file [{}]!", filename);
        throw std::runtime_error(std::format("Unable to open config file [{}]!", filename));
    }

    json config_json;
    file >> config_json;

    MqttConfig config;
    config.host = config_json["mqtt"]["host"];
    config.port = config_json["mqtt"]["port"];
    config.client_id = config_json["mqtt"]["client_id"];

    return config;
}

MqttSubscriber::MqttSubscriber(const std::string& host, uint16_t port, const std::string& client_id,
                               const MessageCallback& callback) :
    ioc_(std::make_unique<boost::asio::io_context>()), client_(*ioc_), host_(host), port_(port), client_id_(client_id),
    work_(boost::asio::make_work_guard(*ioc_)), callback_(callback) {}

void MqttSubscriber::subscribe(const std::string& topic) {
    topic_ = topic;

    client_.brokers(host_, port_).credentials(client_id_)
        .connect_property(boost::mqtt5::prop::session_expiry_interval, 60 * 60) // 1 hour
        .connect_property(boost::mqtt5::prop::maximum_packet_size, 1024 * 1024)  // 1 MB
        .async_run(boost::asio::detached);
;

    const boost::mqtt5::subscribe_topic sub_topic(topic);

    // 订阅主题
    client_.async_subscribe(sub_topic, boost::mqtt5::subscribe_props{},
                            [this, topic](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code> rcs,
                                          boost::mqtt5::suback_props props) {
                                if (!ec && !rcs.empty() && !rcs[0]) {
                                    spdlog::info("Subscribed successfully to topic: {}", topic_);
                                } else {
                                    spdlog::info("Failed to subscribe to topic: {} , error: {}", topic_, ec.message());
                                }
                            });

    spdlog::info("Listening for messages on '{}'...", topic);

    // 开始接收消息
    start_receive();

    // 在新线程中运行 io_context
    runner_thread_ = std::thread([this]() { ioc_->run(); });
}

void MqttSubscriber::stop() {
    if (ioc_) {
        ioc_->stop();
    }

    if (runner_thread_.joinable()) {
        runner_thread_.join();
    }
}

MqttSubscriber::~MqttSubscriber() { stop(); }

void MqttSubscriber::start_receive() {
    client_.async_receive([this](boost::mqtt5::error_code ec, const std::string topic, const std::string payload,
                                 boost::mqtt5::publish_props props) {
        if (!ec) {
            spdlog::info("read {} {}", topic, payload);
            callback_(topic, payload);
            // 继续接收下一条消息
            start_receive();
        } else {
            spdlog::error("Error receiving message: {}", ec.message());
        }
    });
}
