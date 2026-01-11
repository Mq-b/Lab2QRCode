#include "mqtt_client.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

MqttConfig MqttSubscriber::loadMqttConfig(const std::string &filename) {
    using json = nlohmann::json;

    MqttConfig config{};
    json root;

    // 1) 读取文件（容错解析）
    try {
        std::ifstream ifs(filename);
        if (ifs) {
            ifs >> root;
        } else {
            spdlog::warn("无法打开配置文件: {}", filename);
        }
    } catch (const json::parse_error &e) {
        spdlog::error("解析配置文件失败 ({}): {}", filename, e.what());
    } catch (const std::exception &e) { spdlog::error("读取配置文件异常 ({}): {}", filename, e.what()); }

    // 2) 取得 mqtt 节点（不存在则使用空对象）
    json mqtt = root.value("mqtt", json::object());

    // 3) 基本字段：使用默认值
    config.host = mqtt.value("host", std::string("localhost"));
    config.port = static_cast<uint16_t>(mqtt.value("port", 1883));

    bool updated = false;

    // 4) client_id：不存在或为空则生成
    std::string client_id = mqtt.value("client_id", std::string{});
    if (client_id.empty()) {
        client_id = generate_client_id();
        mqtt["client_id"] = client_id;
        updated = true;
        spdlog::info("client_id 不存在，生成ID: {}", client_id);
    }
    config.client_id = std::move(client_id);

    // 5) topic：不存在或为空则设置默认
    std::string topic = mqtt.value("topic", std::string{});
    if (topic.empty()) {
        topic = "test/topic";
        mqtt["topic"] = topic;
        updated = true;
        spdlog::info("topic 不存在，设置默认 topic: {}", topic);
    }
    config.topic = std::move(topic);

    // 6) 仅在有更新时写回
    if (updated) {
        root["mqtt"] = mqtt;
        std::ofstream ofs(filename, std::ios::trunc);
        if (ofs) {
            ofs << root.dump(4);
        } else {
            spdlog::warn("无法打开配置文件以写入: {}", filename);
        }
    }

    spdlog::info(
        "MQTT 配置: Host={}, Port={}, ClientID={}, Topic={}", config.host, config.port, config.client_id, config.topic);

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
