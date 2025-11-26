# Lab2QRCode

**Lab2QRCode** 是一款支持 Windows、Linux 平台的工具，支持将**任意二进制或文本文件**转换为多种**条码图片**，以及将**条码图片**解码回原始文件。

![demo1](./images/demo1.png)

## 特性

- 🔄 **双向转换**：支持文件到条码的编码，以及条码到文件的解码
- 📊 **多格式支持**：支持生成和识别多种一维码和二维码格式
- 🔒 **数据安全**：通过 Base64 编码确保特殊字符的正确处理
- 🖼️ **图像支持**：兼容常见图像格式
- 🎯 **用户友好**：简洁的图形界面操作简单

## 构建

使用 `cmake` 管理项目，依赖三方库：

- **`Qt5`** - 图形界面
- [**`zxing-cpp`**](https://github.com/zxing-cpp/zxing-cpp) - 条码处理核心库
- [**`OpenCV4`**](https://github.com/opencv/opencv) - 图像处理
- [**`Boost.Asio`**](https://www.boost.org/doc/libs/master/doc/html/boost_asio.html) - 网络通信
- [**`spdlog`**](https://github.com/gabime/spdlog) - 日志
- [**`nlohmann_json`**](https://github.com/nlohmann/json) - JSON 数据处理
- [**`mqtt5`**](https://github.com/boostorg/mqtt5) - 消息订阅
- [**`magic_enum`**](https://github.com/Neargye/magic_enum) - 枚举转字符串

使用 Visual Studio 17 或 gcc 工具链构建。

项目还依赖 [`pwsh`](https://github.com/PowerShell/PowerShell/releases/tag/v7.5.4) 终端。

```shell
# windows 安装
winget install pwsh
```

Linux 安装见[微软文档](https://learn.microsoft.com/en-us/powershell/scripting/install/install-powershell-on-linux?view=powershell-7.5)。

```cmake
git clone https://gitee.com/Mq-b/Lab2QRCode
mkdir build
cd build
cmake ..
cmake --build . -j --config Release
```

构建完成后，在 `build\Release\bin\` 目录下会生成 `Lab2QRCode.exe` 可执行文件。

## 支持的条码格式

Lab2QRCode 支持以下多种条码格式的生成和识别：

| 条码类型 | 格式名称 | 类别 | 特点 |
|---------|----------|------|------|
| **QR Code** | QRCode | 二维码 | 高容量、快速读取 |
| **Micro QR Code** | MicroQRCode | 二维码 | 小型QR码变体 |
| **rMQR Code** | RMQRCode | 二维码 | 矩形微QR码 |
| **Aztec Code** | Aztec | 二维码 | 不需要留白边 |
| **Data Matrix** | DataMatrix | 二维码 | 小尺寸标识 |
| **PDF417** | PDF417 | 二维码 | 堆叠式线性码 |
| **MaxiCode** | MaxiCode | 二维码 | 货运包裹使用 |
| **EAN-13** | EAN13 | 一维码 | 商品零售 |
| **EAN-8** | EAN8 | 一维码 | 小型商品 |
| **UPC-A** | UPCA | 一维码 | 北美商品 |
| **UPC-E** | UPCE | 一维码 | UPC压缩格式 |
| **Code 128** | Code128 | 一维码 | 高密度字符集 |
| **Code 39** | Code39 | 一维码 | 字母数字支持 |
| **Code 93** | Code93 | 一维码 | Code39增强版 |
| **Codabar** | Codabar | 一维码 | 血库、图书馆 |
| **ITF** | ITF | 一维码 | 交插二五码 |
| **DataBar** | DataBar | 一维码 | 原RSS码 |
| **DataBar Expanded** | DataBarExpanded | 一维码 | 扩展数据容量 |
| **DataBar Limited** | DataBarLimited | 一维码 | 有限字符集 |
| **DX Film Edge** | DXFilmEdge | 一维码 | 电影胶片边码 |

## 数据完整性保障

为确保数据传输和转换过程中的**兼容性**，程序在生成条码前会对数据进行 **Base64 编码**，而在解码时进行 **Base64 解码**。这一过程可以有效防止特殊字符（如控制字符、中文、换行符等）对数据正确性的影响。

您可以手动选择是否启用或禁用此功能。在默认情况下，启用 Base64 编码功能，可以确保数据在条码转换过程中不受字符集、编码等问题的影响。

如果你希望在自己的项目中使用 `Base64` 编解码功能，可以参考本项目中的实现，具体代码见 [`SimpleBase64.h`](./include/SimpleBase64.h)。

## 软件使用说明

## 贡献

我们欢迎任何形式的贡献，包括但不限于 bug 修复、功能增强、文档改进等。如果您希望为项目做出贡献，请遵循以下步骤：

1. **Fork** 本项目到您的 GitHub 帐户

2. 在您的分支上进行修改

3. 提交 Pull Request

## 许可证

本项目采用 [MIT](./LICENSE) 许可证。
