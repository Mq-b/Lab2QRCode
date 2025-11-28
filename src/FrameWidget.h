#pragma once
#include <QWidget>
#include <opencv2/core.hpp>

class FrameWidget : public QWidget {
    Q_OBJECT
public:
    explicit FrameWidget(QWidget *parent = nullptr);

    // 接受外部 Mat（必须是 BGR888）
    // 注意：函数内部会复制 Mat 数据（安全方式）
    void setFrame(const cv::Mat &bgr);
    
    void clear();
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;      // 转换后的图像
};