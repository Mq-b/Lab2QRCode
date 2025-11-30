#include "FrameWidget.h"
#include <QPainter>
#include <QStyleOption>
#include <qnamespace.h>


QRect FrameWidget::scaleKeepAspect(const QRect &outer, int w, int h) const {
  if (w <= 0 || h <= 0)
    return {};

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

  return QRect(outer.x() + (outerW - newW) / 2, outer.y() + (outerH - newH) / 2,
               newW, newH);
}


FrameWidget::FrameWidget(QWidget *parent)
    : QWidget(parent)

{
  setStyleSheet("QWidget{border:1px solid black; background-color:black;}");
}

void FrameWidget::setFrame(const QImage &frame) {
  m_image = frame; 
  update();               
}

void FrameWidget::setBarcodeResult(const FrameResult &r) {
  m_barcodeResult = r;
  update(); 
}

void FrameWidget::clear() {
  m_image = QImage();
  update();
}
void FrameWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 绘制背景
    QStyleOption opt;
    opt.init(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    if (m_image.isNull())
        return;

    // 绘制视频帧
    QRect dst = scaleKeepAspect(rect(), m_image.width(), m_image.height());
    painter.drawImage(dst, m_image);

    // 绘制二维码四边形
    if (!m_barcodeResult.points.isEmpty()) {
        painter.setPen(QPen(Qt::green, 2));

        float scaleX = float(dst.width()) / float(m_image.width());
        float scaleY = float(dst.height()) / float(m_image.height());

        QPolygonF poly;
        for (auto &pt : m_barcodeResult.points) {
            poly << QPointF(dst.left() + pt.x() * scaleX,
                            dst.top()  + pt.y() * scaleY);
        }

        painter.drawPolygon(poly);
        painter.setPen(QPen(Qt::black, 3));
        painter.drawText(poly.boundingRect().topLeft() + QPointF(0, -2),
                         m_barcodeResult.content);
    }
}