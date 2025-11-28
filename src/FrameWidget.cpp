#include "FrameWidget.h"
#include <QPainter>
#include <QStyleOption>
#include <spdlog/spdlog.h>
#include <QImage>
namespace {

// 输入 outer rect 和图像宽高，返回居中等比缩放后的 rect
QRect scaleKeepAspect(const QRect &outer, int w, int h)
{
    if (w <= 0 || h <= 0) return {};

    const float outerW = outer.width();
    const float outerH = outer.height();
    const float imgRatio = float(w) / float(h);
    const float viewRatio = outerW / outerH;

    int newW, newH;
    if (imgRatio > viewRatio) {
        newW = outerW;
        newH = outerW / imgRatio;
    } else {
        newH = outerH;
        newW = outerH * imgRatio;
    }

    return QRect(
        outer.x() + (outerW - newW) / 2,
        outer.y() + (outerH - newH) / 2,
        newW, newH
    );
}

}

FrameWidget::FrameWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("QWidget{border:1px solid black; background-color:black;}");
}

//
// ⭐ 接收 BGR888 Mat 并触发绘制
//
void FrameWidget::setFrame(const cv::Mat &bgr)
{
    if (bgr.empty() || bgr.type() != CV_8UC3) {
        spdlog::warn("PlayerWidget::setFrame received invalid mat");
        return;
    }

    // 使用 QImage（深拷贝保证安全）
    // QImage(const uchar*,int,int,int,Format) 不会深拷贝，需要 copy()
    m_image = QImage(
        bgr.data, bgr.cols, bgr.rows, bgr.step,
        QImage::Format_BGR888
    ).copy();   // 🔥 必须 copy()，否则 rgb 临时变量析构后数据失效

    update();   // 触发 Qt 重绘
}

//
// ⭐ 根据 Widget 大小自动缩放绘制
//
void FrameWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 绘制背景（保持 Qt 的 style 支持）
    QStyleOption opt;
    opt.init(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // 没有图像直接返回
    if (m_image.isNull())
        return;

    // 自动等比缩放并居中
    QRect dst = scaleKeepAspect(rect(), m_image.width(), m_image.height());

    painter.drawImage(dst, m_image);
}


void FrameWidget::clear()
{
    m_image = QImage(); // 清空图像
    update();           // 触发重绘
}