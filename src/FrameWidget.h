#pragma once
#include <QWidget>
#include <QImage>
#include <ZXing/ReadBarcode.h>
#include "commondef.h"
class FrameWidget : public QWidget {
    Q_OBJECT
public:
    explicit FrameWidget(QWidget *parent = nullptr);

void setFrame(const QImage& frame);           // 设置视频帧
void setBarcodeResult(const FrameResult& r);  // 设置二维码信息
    
    void clear();
protected:
    void paintEvent(QPaintEvent *event) override;


private:
    QRect scaleKeepAspect(const QRect &outer, int w, int h) const;


private:
    QImage m_image; 
    FrameResult m_barcodeResult;
};
