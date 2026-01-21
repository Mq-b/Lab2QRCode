# MCP 服务器配置说明

本项目配置了以下 MCP (Model Context Protocol) 服务器：

## 配置的服务器

### 1. fetch (通用 Web 获取)
- **类型**: stdio
- **命令**: `npx -y @modelcontextprotocol/server-fetch`
- **用途**: 获取任何网页内容，转换为 Markdown 格式
- **使用场景**:
  - 获取在线文档
  - 读取 API 文档
  - 访问技术博客

### 2. cppreference-docs (C++ 参考文档)
- **类型**: stdio
- **命令**: `npx -y @modelcontextprotocol/server-puppeteer`
- **限制域名**: en.cppreference.com, cppreference.com
- **用途**: 专门用于访问 C++ 参考文档网站
- **使用场景**:
  - 学习 C++ 标准库接口
  - 查询函数用法和示例
  - 了解 C++17/20/23 新特性

## 使用方法

### 查询 C++ 接口文档

当你需要查询 C++ 标准库的某个功能时，可以这样请求：

```
请帮我查询 std::format 的用法
```

```
如何使用 std::optional?
```

```
C++20 的 ranges 库有哪些视图?
```

Claude Code 会自动使用 MCP 服务器访问 cppreference.com 获取最新的文档信息。

### 常用查询示例

1. **标准库容器**
   - "std::vector 的所有成员函数"
   - "std::unordered_map vs std::map 的区别"
   - "std::array 如何初始化"

2. **智能指针**
   - "std::unique_ptr 的用法"
   - "std::shared_ptr 如何工作"
   - "std::weak_ptr 的使用场景"

3. **C++17/20 新特性**
   - "std::optional 详细说明"
   - "std::variant 使用示例"
   - "structured bindings 语法"
   - "ranges 库的 transform 视图"

4. **字符串和格式化**
   - "std::format 格式说明符"
   - "std::string_view 性能优势"
   - "std::to_string 替代方案"

5. **并发和线程**
   - "std::thread 使用方法"
   - "std::mutex 和 std::lock_guard"
   - "std::atomic 操作"

## 配置文件位置

- **项目级配置**: `d:\project\Lab2QRCode\.claude\mcp.json`
- **全局配置** (可选): `%APPDATA%\Claude\mcp.json`

## 生效方式

1. 保存配置文件后，重启 Claude Code
2. 或者重新加载 VS Code 窗口 (Ctrl+Shift+P → "Reload Window")
3. MCP 服务器会自动连接并可用

## 验证配置

配置生效后，你可以直接询问 C++ 相关问题，Claude Code 会自动：
1. 检测到需要查询文档
2. 使用 MCP 服务器访问 cppreference.com
3. 获取最新、最准确的信息
4. 结合项目代码上下文给出建议

## 依赖要求

- **Node.js**: 需要安装 Node.js (用于运行 npx 命令)
- **网络连接**: 需要能访问 cppreference.com

检查 Node.js:
```bash
node --version
npm --version
```

如果未安装，请访问: https://nodejs.org/

## 故障排查

### MCP 服务器未连接

1. 检查 Node.js 是否已安装
2. 确认网络连接正常
3. 查看 Claude Code 日志中的错误信息
4. 重启 VS Code 或 Claude Code

### 查询失败

1. 确认 cppreference.com 可访问
2. 检查防火墙设置
3. 尝试手动访问网站确认可用性

## 扩展建议

如果需要访问其他 C++ 相关网站，可以在 `mcp.json` 中添加：

```json
{
  "mcpServers": {
    "fetch": { ... },
    "cppreference-docs": { ... },
    "cpp-guidelines": {
      "type": "stdio",
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-fetch"],
      "env": {
        "ALLOWED_DOMAINS": "isocpp.github.io"
      }
    }
  }
}
```

这样就可以同时访问：
- cppreference.com (C++ 参考文档)
- isocpp.github.io (C++ Core Guidelines)
- 其他技术网站
