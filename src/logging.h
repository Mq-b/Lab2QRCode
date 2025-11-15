#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
namespace Logging {
/**
 * @brief 初始化 spdlog 日志系统
 *
 * 该函数会完成以下操作：
 * 1. 获取应用程序所在目录，并在其下创建 logs 子目录
 * 2. 配置日志输出到：
 *      - 彩色终端（stdout）
 *      - 日志文件 logs/app.log
 * 3. 设置默认 logger，使得 spdlog::info/debug/error 等全局可用
 * 4. 设置日志等级为 debug（可记录 debug 及以上级别）
 * 5. 设置 flush_on(info)，保证 info 及以上日志立即写入文件
 * 6. 捕获初始化异常并打印到 stderr
 *
 * @note 日志目录总是相对于应用程序可执行文件所在路径，不依赖工作目录
 */
inline void initSpdlog()
{
    // 获取应用所在目录，例如 .../build/app/Debug/
    QString appDir = QCoreApplication::applicationDirPath();
    QString logDir = appDir + "/logs";

    // 创建 logs 目录
    QDir().mkpath(logDir);

    // 最终日志文件路径
    QString logFile = logDir + "/app.log";

    try
    {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            logFile.toStdString(), false));

        auto logger = std::make_shared<spdlog::logger>("app", sinks.begin(), sinks.end());

        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::info);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        fprintf(stderr, "Log init failed: %s\n", ex.what());
    }

    spdlog::info("application working at: ",QDir().currentPath().toStdString());
}

}