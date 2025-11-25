# MQTT

## MQTT 协议简介

MQTT 是一个 客户端/服务器 架构，采用 订阅/发布 模式的消息传输协议。是一套轻量级的、开放的、简单且易于实现的标准。这些特性使得他适用于多种场景，包括一些资源受限的场景比如机器和机器之间的通信（M2M）或是物联网（IoT）场景，这些场景要求较小的代码空间占用，或是网络带宽非常珍贵。

**MQTT 基于 TCP/IP** 或其他提供了顺序、无包丢失、双向链接的网络协议。MQTT 的特性包括：

- 通过 订阅/发布 模式实现一对多的消息传输和应用程序解耦。
- 与负载内容无关的消息传输。
- 三种不同服务质量(QoS)的消息传输：
  - **`Qos0`** 至多一次(At most once)，根据操作环境情况尽最大努力来传输消息，消息可能会丢失。例如这种模式可以用于传感器数据采集，单次的消息的丢失并不重要，因为下一个消息很快就会到来。
  - **`Qos1`** 至少一次(At least once)，可以确保消息到达，但是可能会造成消息重复。
  - **`Qos2`** 确保一次(Exactly once)，可以确保消息只到达一次，例如这种消息可以用于账单交易信息，在交易场景下消息的丢失或者重复处理都会带来糟糕的后果。
- 小型的协议头，用来降低网络负载。
- 当发生异常断开时通知相关方的机制。

---

> [!NOTE]
> 简而言之，MQTT 协议是一种**轻量化**的、基于 **TCP/IP** 实现的、采用**订阅/发布**模式的通信协议。它使用 **MQTT 代理**（Broker）作为消息中枢，实现彻底解耦。**客户端向代理的特定主题（Topic）发布（Publish）消息，代理则负责将其精准路由给所有订阅（Subscribe）了该主题的客户端。**

> [!TIP]
> 关于 MQTT 协议，详情可见 [MQTT5.0 中文文档](https://vitsumoc.github.io/mqtt-v5-0-chinese.html)。

## MQTT 代理

使用 [**mosquitto**](https://github.com/eclipse-mosquitto/mosquitto) 开源代理项目，简单的在服务器上进行配置。

以 Ubuntu 为例[安装](https://mosquitto.org/download/)命令：

```shell
sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
sudo apt update
sudo apt install mosquitto mosquitto-clients
```

修改配置文件 `/etc/mosquitto/mosquitto.conf`，增加内容，例如：

```conf
# 配置监听器
listener 1883 0.0.0.0

# 允许匿名访问
allow_anonymous true

# 设置默认的日志级别
log_dest stdout
```

同时云服务器后台需要开放 `1883` 端口。

我们可以使用命令 `sudo tail -f /var/log/mosquitto/mosquitto.log` 来查看 mqtt 代理的运行日志。


### 客户端订阅与发布

使用 [**mosquitto**](https://github.com/eclipse-mosquitto/mosquitto) 提供的 `mosquitto_sub` 与 `mosquitto_pub` 工具简单演示一下。

客户端订阅消息：

```shell
.\mosquitto_sub.exe -h 127.0.0.1 -t 'test/topic' -v
```

客户端发布订阅：

```shell
mosquitto_pub -t 'test/topic' -m 'hello world'
```

订阅的客户端会收到发布客户端发布的订阅，并输出：

```shell
test/topic hello world
```

## 使用 mqtt5 编写代码

[`Boost.MQTT5`](https://github.com/boostorg/mqtt5)：一个基于 `Boost.Asio` 的 C++17 MQTT 客户端库。

它完整实现了 MQTT 5.0 协议，开发者可通过该库在应用中轻松集成消息发布和订阅能力。

```shell
mosquitto_pub -t 'test/topic' -m 'hello world' -q 2
```

即使客户端掉线，消息也不会丢失，在客户端上线后，会收到该消息。
