#include <QCoreApplication>
#include <filesystem>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Logging {

/**
     * @brief 初始化日志系统
     *
     * 配置 spdlog，使用文件和控制台两个输出通道。
     * 日志文件按天滚动保存，控制台输出带颜色。
     */
inline void setupLogging() {
    if (std::filesystem::exists("Log")) {
        std::filesystem::create_directory("Log");
    }
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("Log/daily.log", 0, 0);
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [thread %t] [%oms] [%l] %v");

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S] [thread %t] [%oms] [%l] %v%$");

    auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{file_sink, console_sink});
    spdlog::register_logger(logger);

    spdlog::set_default_logger(logger);
    spdlog::flush_on(spdlog::level::debug);
}

} // namespace Logging
