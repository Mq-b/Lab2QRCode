#pragma once 
#include <QString>
#include <opencv2/core/mat.hpp>

struct FrameResult
{
    cv::Mat frame; 
    bool hasBarcode = false;
    QString type;
    QString content;
};