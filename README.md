# Lab2QRCode

**Lab2QRCode** 是一款支持 Windows、Linux 平台的工具，支持将**任意二进制或文本文件**转换为多种**条码图片**，以及将**条码图片**解码回原始文件。

![demo1](./images/demo1.png)

## 特性

- 🔄 **双向转换**：支持文件（包括二进制文件）到条码的编码，以及条码到文件的解码
- 📊 **多格式支持**：支持生成和识别几乎所有的标准一维二维条码格式
- 🔒 **数据安全**：通过 Base64 编码确保特殊字符的正确处理
- 🖼️ **图像支持**：兼容常见图像格式
- 🎯 **用户友好**：简洁的图形界面，操作简单直观
- 📂 **批量处理**：支持一次性处理多个文件，提升工作效率
- ✏️ **手动输入生成条码**：用户可手动输入文本生成条码
- 📷 **摄像头扫描识别**：支持使用摄像头扫描条码进行识别和解码

| ![单文件生成与解码](images/单文件生成和解码.gif) | ![手动输入生成条码](images/手动输入生成条码.gif) |                                            ![批量文件生成条码](images/批量生成和解码.gif)                                            |
|:------------------------------------------------:|:------------------------------------------------:|:------------------------------------------------------------------------------------------------------------------------------------:|
|               **单文件生成与解码**               |               **手动输入生成条码**               | **批量文件生成条码**<br />点击浏览按钮后打开目录，可多选文件。<br />解码不在乎条码类型选择的是什么，默认尝试所有 19 种条码进行解码。 |

### 摄像头扫描识别

<https://github.com/user-attachments/assets/d6fbb77c-bed0-4dca-acd7-50650591413e>

## 构建

使用 `cmake` 管理项目，依赖三方库：

- [**`Qt5`**](https://doc.qt.io/archives/qt-5.12/) - 图形界面
- [**`zxing-cpp`**](https://github.com/zxing-cpp/zxing-cpp) - 条码处理核心库
- [**`OpenCV4`**](https://github.com/opencv/opencv) - 图像处理
- [**`Boost.Asio`**](https://www.boost.org/doc/libs/master/doc/html/boost_asio.html) - 网络通信
- [**`spdlog`**](https://github.com/gabime/spdlog) - 日志
- [**`nlohmann_json`**](https://github.com/nlohmann/json) - JSON 数据处理
- [**`mqtt5`**](https://github.com/boostorg/mqtt5) - 消息订阅
- [**`magic_enum`**](https://github.com/Neargye/magic_enum) - 枚举转字符串
- [**`xlsxwriter`**](https://github.com/jmcnamara/libxlsxwriter) - Excel 文件写入

使用 Visual Studio 17 或 gcc 工具链构建。

项目还依赖 [`pwsh`](https://github.com/PowerShell/PowerShell/releases/tag/v7.5.4) 终端。

### Linux

#### 安装 PowerShell

在 Debian 系发行版上，可以使用以下命令安装：

```sh
curl -O "https://mirrors.pku.edu.cn/debian/pool/main/i/icu/libicu72_72.1-3+deb12u1_amd64.deb"
sudo dpkg -i libicu72_72.1-3+deb12u1_amd64.deb
curl -O "https://github.com/PowerShell/PowerShell/releases/download/v7.5.4/powershell_7.5.4-1.deb_amd64.deb"
sudo dpkg -i powershell_7.5.4-1.deb_amd64.deb
```

其它发行版参照[微软文档](https://learn.microsoft.com/en-us/powershell/scripting/install/install-powershell-on-linux?view=powershell-7.5)。

#### 安装依赖库

在 Debian 系发行版上，可以使用以下命令安装依赖：

```sh
sudo apt install qtbase5-dev qt5-qmake qtmultimedia5-dev libboost-all-dev cmake ninja-build build-essential libopencv-dev libspdlog-dev libxlsxwriter-dev libzxing-dev
```

> 注意：如果是使用了 `clang >= 20` 编译器，则不应该安装 `libfmt-dev` 和 `libspdlog-dev`。
>
> 请自行从源码编译 `fmt` 和 `spdlog`，因为它们在新版修复了一个编译问题。

对于其它发行版，请自行安装相应的依赖包。

#### 构建项目

```sh
git clone https://github.com/Mq-b/Lab2QRCode
mkdir build && cd build
cmake ..
cmake --build . -j --config Release
```

构建完成后，在 `build/bin/` 目录下会生成 `Lab2QRCode` 可执行文件。

### Windows

#### 安装 PowerShell

```sh
winget install pwsh
```

#### 安装依赖库

可以使用 `vcpkg` 来安装依赖库，具体请自行处理。

`vcpkg` 安装方法如下：

```sh
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install opencv[core]:x64-windows spdlog:x64-windows libxlsxwriter:x64-windows
```

> 注意这没有安装 `qt5` `boost` 和 `zxing-cpp`，请自行安装它们。

#### 构建项目

```sh
git clone https://github.com/Mq-b/Lab2QRCode
mkdir build && cd build
cmake ..
cmake --build . -j --config Release
```

> 如果你在上一步中安装了 `vcpkg`，请将：
>
> ```sh
> cmake ..
> ```
>
> 替换为：
>
> ```sh
> cmake .. -DCMAKE_TOOLCHAIN_FILE="../vcpkg/scripts/buildsystems/vcpkg.cmake"
> ```

构建完成后，在 `build\Release\bin\` 目录下会生成 `Lab2QRCode.exe` 可执行文件。

## 支持的条码格式

Lab2QRCode 支持以下多种条码格式的生成和识别：

| 条码类型             | 格式名称        | 类别   | 特点             |
|----------------------|-----------------|--------|------------------|
| **QR Code**          | QRCode          | 二维码 | 高容量、快速读取 |
| **Micro QR Code**    | MicroQRCode     | 二维码 | 小型QR码变体     |
| **rMQR Code**        | RMQRCode        | 二维码 | 矩形微QR码       |
| **Aztec Code**       | Aztec           | 二维码 | 不需要留白边     |
| **Data Matrix**      | DataMatrix      | 二维码 | 小尺寸标识       |
| **PDF417**           | PDF417          | 二维码 | 堆叠式线性码     |
| **MaxiCode**         | MaxiCode        | 二维码 | 货运包裹使用     |
| **EAN-13**           | EAN13           | 一维码 | 商品零售         |
| **EAN-8**            | EAN8            | 一维码 | 小型商品         |
| **UPC-A**            | UPCA            | 一维码 | 北美商品         |
| **UPC-E**            | UPCE            | 一维码 | UPC压缩格式      |
| **Code 128**         | Code128         | 一维码 | 高密度字符集     |
| **Code 39**          | Code39          | 一维码 | 字母数字支持     |
| **Code 93**          | Code93          | 一维码 | Code39增强版     |
| **Codabar**          | Codabar         | 一维码 | 血库、图书馆     |
| **ITF**              | ITF             | 一维码 | 交插二五码       |
| **DataBar**          | DataBar         | 一维码 | 原RSS码          |
| **DataBar Expanded** | DataBarExpanded | 一维码 | 扩展数据容量     |
| **DataBar Limited**  | DataBarLimited  | 一维码 | 有限字符集       |
| **DX Film Edge**     | DXFilmEdge      | 一维码 | 电影胶片边码     |

## 数据完整性保障

为确保数据传输和转换过程中的**兼容性**，程序在生成条码前会对数据进行 **Base64 编码**，而在解码时进行 **Base64 解码**。这一过程可以有效防止特殊字符（如控制字符、中文、换行符等）对数据正确性的影响。

您可以手动选择是否启用或禁用此功能。在默认情况下，启用 Base64 编码功能，可以确保数据在条码转换过程中不受字符集、编码等问题的影响。

如果你希望在自己的项目中使用 `Base64` 编解码功能，可以参考本项目中的实现，具体代码见 [`SimpleBase64.h`](./include/SimpleBase64.h)。

## 贡献

我们欢迎任何形式的贡献，包括但不限于 bug 修复、功能增强、文档改进等。如果您希望为项目做出贡献，请遵循以下步骤：

1. **Fork** 本项目到您的 GitHub 帐户

2. 在您的分支上进行修改

3. 提交 Pull Request

### 代码格式化

我们使用 `clang-format` 来统一代码风格。请确保在提交代码前执行自动格式化。

CI 使用 `clang-format-22` 进行检查，建议使用相同版本的 `clang-format` 来格式化代码。

> 注意如果使用 `clang-format-19` 或更低版本有一个已知问题：
>
> 在低版本中：
>
> ```cpp
> MyClass obj = {
>        .member1 = value1,
>        .member2 = value2,
> };
> ```
>
> 然而在高版本中：
>
> ```cpp
> MyClass obj = {
>     .member1 = value1,
>     .member2 = value2,
> };
> ```
>
> 因此建议使用 `clang-format-22` 以避免不必要的代码差异。

如果你是 Debian 系发行版用户，可以使用以下命令快速安装 `clang-22` 工具链：

```sh
sudo apt install curl gnupg
curl https://apt.llvm.org/llvm.sh | sudo bash -s 22 all
```

如果你只希望安装 `clang-format-22`，可以使用以下命令：

```sh
sudo apt install curl gnupg
curl https://apt.llvm.org/llvm.sh | grep -Ev '^\s*apt(-get)?\s+install' | sudo bash -s 22
sudo apt install clang-format-22
```

## 许可证

本项目采用 [MIT](./LICENSE) 许可证。
