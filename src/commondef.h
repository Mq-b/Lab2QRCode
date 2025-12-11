#pragma once
#include <QString>
#include <opencv2/core/mat.hpp>

/**
 * @brief 结构体表示一帧图像及其二维码扫描结果
 */
struct FrameResult {
    cv::Mat frame;
    cv::Mat rectifiedImage;
    bool hasBarcode = false;
    QString type;
    QString content;
};