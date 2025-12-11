#include "CameraConfig.h"
#include "sysinfo.h"
#include <QCamera>
#include <QCameraInfo>
#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

CameraConfig::operator QString() const {
    return QString("%1x%2 @ %3 fps (%4)").arg(width).arg(height).arg(fps).arg(pixelFormat);
}

CameraConfig::operator std::string() const {
    return std::format("{}x{} @ {} fps ({})", width, height, fps, pixelFormat.toStdString());
}

QStringList CameraConfig::getCameraDescriptions() {
    const auto cameras = QCameraInfo::availableCameras();
    QStringList descriptions;
    for (const auto &camInfo : cameras) {
        descriptions.append(camInfo.description());
    }
    return descriptions;
}

std::vector<CameraConfig> CameraConfig::getSupportedCameraConfigs(int cameraIndex) {
    std::vector<CameraConfig> configs;
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();

    if (cameraIndex >= cameras.size()) {
        return configs;
    }

    const QCameraInfo cameraInfo = cameras[cameraIndex];
    spdlog::info("Camera name: {}", cameraInfo.description().toStdString());

    QCamera camera(cameraInfo);
    camera.start();

    // 现在查询支持的设置
    QList<QCameraViewfinderSettings> supportedSettings = camera.supportedViewfinderSettings();
    spdlog::info("Available viewfinder settings: {}", supportedSettings.size());

    for (const QCameraViewfinderSettings &settings : supportedSettings) {
        CameraConfig config;
        config.width = settings.resolution().width();
        config.height = settings.resolution().height();
        config.fps = settings.maximumFrameRate();
        config.pixelFormat = magic_enum::enum_name(settings.pixelFormat()).data();

        // 添加到列表
        configs.emplace_back(config);

        spdlog::info("Supported config: {}", static_cast<std::string>(config));
    }

    camera.stop(); // 查询完成，停止摄像头

    return configs;
}

CameraConfig CameraConfig::selectBestCameraConfig(const std::vector<CameraConfig> &configs) {
    // 1. 如果没有配置，返回默认配置
    if (configs.empty()) {
        return {640, 480, 30, "Unknown"};
    }

    // 2. 如果系统内存 >= 8GB 且 CPU 核心 >= 8，选择分辨率最大的配置（帧数高优先）
    const auto ramGB = sysinfo::getSystemRAM<sysinfo::GB>();
    const auto cpuCores = sysinfo::getCPUCoreCount();
    if (ramGB >= 8 && cpuCores >= 8) {
        const auto best = std::ranges::max_element(configs, [](const CameraConfig &a, const CameraConfig &b) {
            const int resA = a.width * a.height;
            const int resB = b.width * b.height;
            if (resA == resB) {
                return a.fps < b.fps; // 分辨率相同，选择帧数高的
            }
            return resA < resB;
        });
        return *best;
    }

    // 3. 优先选择 1920x1080，如果多个相同分辨率，选择帧数最高的
    std::vector<CameraConfig> fullHDConfigs;
    for (const auto &cfg : configs) {
        if (cfg.width == 1920 && cfg.height == 1080) {
            fullHDConfigs.push_back(cfg);
        }
    }
    if (!fullHDConfigs.empty()) {
        const auto best = std::ranges::max_element(
            fullHDConfigs, [](const CameraConfig &a, const CameraConfig &b) { return a.fps < b.fps; });
        return *best;
    }

    // 4. 尝试选择 640x480 帧数
    for (const auto &cfg : configs) {
        if (cfg.width == 640 && cfg.height == 480) {
            return cfg;
        }
    }

    // 5. 如果以上都不符合，返回支持的最小分辨率
    return configs.front();
}
