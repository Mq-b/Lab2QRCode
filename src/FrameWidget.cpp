#include "FrameWidget.h"
#include <QImage>
#include <QPainter>
#include <QStyleOption>
#include <spdlog/spdlog.h>
namespace {

// 输入 outer rect 和图像宽高，返回居中等比缩放后的 rect
QRect scaleKeepAspect(const QRect &outer, const int w, const int h) {
    if (w <= 0 || h <= 0) {
        return {};
    }

    const float outerW = outer.width();
    const float outerH = outer.height();
    const float imgRatio = static_cast<float>(w) / static_cast<float>(h);
    const float viewRatio = outerW / outerH;

    int newW, newH;
    if (imgRatio > viewRatio) {
        newW = outerW;
        newH = outerW / imgRatio;
    } else {
        newH = outerH;
        newW = outerH * imgRatio;
    }

    return QRect(outer.x() + (outerW - newW) / 2, outer.y() + (outerH - newH) / 2, newW, newH);
}

} // namespace

FrameWidget::FrameWidget(QWidget *parent)
    : QWidget(parent) {
    setStyleSheet("QWidget{border:1px solid black; background-color:black;}");
}

void FrameWidget::setFrame(const cv::Mat &bgr) {
    if (bgr.empty() || bgr.type() != CV_8UC3) {
        spdlog::warn("PlayerWidget::setFrame received invalid mat");
        return;
    }

    // 创建 RGB QImage 然后交换 R 和 B 通道
    m_image = QImage(bgr.data,
                     bgr.cols,
                     bgr.rows,
                     static_cast<int>(bgr.step),
                     QImage::Format_RGB888)
                  .rgbSwapped()
                  .copy(); // 先交换通道再深拷贝

    update(); // 触发 Qt 重绘
}

void FrameWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 绘制背景（保持 Qt 的 style 支持）
    QStyleOption opt;
    opt.init(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // 没有图像直接返回
    if (m_image.isNull()) {
        return;
    }

    // 自动等比缩放并居中
    const QRect dst = scaleKeepAspect(rect(), m_image.width(), m_image.height());

    painter.drawImage(dst, m_image);
}

void FrameWidget::clear() {
    m_image = QImage(); // 清空图像
    update();           // 触发重绘
}