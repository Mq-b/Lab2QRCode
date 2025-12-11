#include "mqtt_client.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

MqttConfig MqttSubscriber::loadMqttConfig(const std::string &filename) {
    std::ifstream file(filename);
    json config_json;

    if (file.is_open()) {
        file >> config_json;
    }

    // 默认配置,如果配置文件不存在则使用默认值
    std::string host = "127.0.0.1";
    uint16_t port = 1883;
    std::string client_id;

    // 文件存在则更新上面的默认值
    if (config_json.contains("mqtt") && config_json["mqtt"].is_object()) {
        auto &mqtt = config_json["mqtt"];
        if (mqtt.contains("host") && mqtt["host"].is_string()) {
            host = mqtt["host"].get<std::string>();
        }
        if (mqtt.contains("port") && mqtt["port"].is_number()) {
            port = mqtt["port"].get<uint16_t>();
        }
        if (mqtt.contains("client_id") && mqtt["client_id"].is_string()) {
            client_id = mqtt["client_id"].get<std::string>();
        }
    }

    spdlog::info("MQTT 配置文件: Host={}, Port={}, Client ID={}", host, port, client_id);

    MqttConfig config;
    config.host = host;
    config.port = port;

    // 如果 client_id 不存在或为空，则生成并保存
    if (client_id.empty()) {
        config.client_id = generate_client_id();
        spdlog::info("client_id 不存在，生成ID: {}", config.client_id);

        // 更新配置
        config_json["mqtt"]["host"] = host;
        config_json["mqtt"]["port"] = port;
        config_json["mqtt"]["client_id"] = config.client_id;

        std::ofstream out_file(filename);
        if (out_file.is_open()) {
            out_file << config_json.dump(4);
        }
    } else {
        config.client_id = client_id;
    }

    return config;
}

std::string MqttSubscriber::generate_client_id(const std::string &app_name) {
    static boost::uuids::random_generator generator;
    const boost::uuids::uuid uuid = generator();
    return app_name + "_" + boost::uuids::to_string(uuid);
}

MqttSubscriber::MqttSubscriber(const std::string &host,
                               uint16_t port,
                               const std::string &client_id,
                               const MessageCallback &callback)
    : ioc_(std::make_unique<boost::asio::io_context>()),
      client_(*ioc_),
      host_(host),
      port_(port),
      client_id_(client_id),
      work_(boost::asio::make_work_guard(*ioc_)),
      callback_(callback) {}

void MqttSubscriber::subscribe(const std::string &topic) {
    topic_ = topic;

    client_.brokers(host_, port_)
        .credentials(client_id_)
        .connect_property(boost::mqtt5::prop::session_expiry_interval, 60 * 60) // 1 hour
        .connect_property(boost::mqtt5::prop::maximum_packet_size, 1024 * 1024) // 1 MB
        .async_run(boost::asio::detached);
    ;

    const boost::mqtt5::subscribe_topic sub_topic(topic);

    // 订阅主题
    client_.async_subscribe(sub_topic,
                            boost::mqtt5::subscribe_props{},
                            [this, topic](boost::mqtt5::error_code ec,
                                          std::vector<boost::mqtt5::reason_code> rcs,
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
    runner_thread_ = std::thread([this] { ioc_->run(); });
}

void MqttSubscriber::stop() {
    if (ioc_) {
        ioc_->stop();
    }

    if (runner_thread_.joinable()) {
        runner_thread_.join();
    }
}

MqttSubscriber::~MqttSubscriber() {
    stop();
}

void MqttSubscriber::start_receive() {
    client_.async_receive([this](boost::mqtt5::error_code ec,
                                 const std::string topic,
                                 const std::string payload,
                                 boost::mqtt5::publish_props props) {
        if (!ec) {
            callback_(topic, payload);
            // 继续接收下一条消息
            start_receive();
        } else {
            spdlog::error("Error receiving message: {}", ec.message());
        }
    });
}
