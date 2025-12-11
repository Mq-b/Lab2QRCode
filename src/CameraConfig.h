#pragma once

#include <QString>
#include <format>
#include <vector>

struct CameraConfig {
    int width;
    int height;
    int fps;
    QString pixelFormat;

    operator QString() const;

    operator std::string() const;

    /**
     * @brief 获取系统中可用摄像头的描述列表
     *
     * @return 摄像头描述字符串列表
     */
    static QStringList getCameraDescriptions();

    /**
     * @brief 获取指定摄像头支持的配置列表
     *
     * @param cameraIndex 摄像头设备索引
     * @return 支持的摄像头配置列表
     */
    static std::vector<CameraConfig> getSupportedCameraConfigs(int cameraIndex);

    /**
     * @brief 从支持的摄像头配置中选择最佳配置
     *
     * @param configs 支持的摄像头配置列表
     * @return 选择的最佳摄像头配置
     */
    static CameraConfig selectBestCameraConfig(const std::vector<CameraConfig> &configs);
};