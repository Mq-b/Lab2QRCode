#pragma once
#include <QCameraInfo>
#include <QCameraImageCapture>
#include <QTimer>
#include <QPainter>
#include <ZXing/ReadBarcode.h>
#include <QWidget>
#include <QAbstractVideoSurface>
#include <QVideoFrame>
#include <QAbstractVideoBuffer>
#include <QMenuBar>
#include <QStatusBar>
#include <QTableView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QLabel>
#include <QSharedPointer>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <memory>
#include <spdlog/spdlog.h>

#include "FrameWidget.h"
#include "commondef.h" // FrameResult

// ----------------------
// CameraVideoSurface
// ----------------------
class CameraVideoSurface : public QAbstractVideoSurface {
    Q_OBJECT
public:
    explicit CameraVideoSurface(QObject* parent = nullptr)
        : QAbstractVideoSurface(parent) {}

    // We emit a shared pointer to avoid unnecessary copies when possible.
    bool present(const QVideoFrame& frame) override {
        if (!frame.isValid()) return false;

        QVideoFrame clone(frame);
        if (!clone.map(QAbstractVideoBuffer::ReadOnly)) return false;

        QImage::Format qfmt = QVideoFrame::imageFormatFromPixelFormat(clone.pixelFormat());
        if (qfmt == QImage::Format_Invalid) {
            clone.unmap();
            return false;
        }

        // Copy is required because the QVideoFrame-backed memory will be unmapped.
        QImage img(clone.bits(), clone.width(), clone.height(), clone.bytesPerLine(), qfmt);

        clone.unmap();

        emit frameReady(img);
        return true;
    }

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle
    ) const override {
        Q_UNUSED(handleType);
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_ARGB32
                                                 << QVideoFrame::Format_RGB32
                                                 << QVideoFrame::Format_RGB24;
    }

signals:
    void frameReady(const QImage& img);

};

// ----------------------
// CameraWidget
// ----------------------
class CameraWidget : public QWidget {
    Q_OBJECT
public:
    CameraWidget(QWidget* parent = nullptr);
    ~CameraWidget();

    void startCamera(int camIndex = 0);
    void stopCamera();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onCameraIndexChanged(int index);
    void onFrameReady(const QImage& img);

    void onScanFinished();

private:
    FrameResult processFrameSync(QImage img);
    ZXing::ImageView ImageViewFromQImage(const QImage &img);

private:
    QVBoxLayout* mainLayout = nullptr;
    QMenuBar* menuBar = nullptr;
    QMenu* cameraMenu = nullptr;

    QVector<QCamera*> camerasList;
    CameraVideoSurface* videoSurface = nullptr; // owned by this widget
    int currentCameraIndex = 0;
    QCamera* currentCamera = nullptr;
    bool cameraStarted = false;

    FrameWidget* frameWidget = nullptr; // 用 QImage 绘制
    QStandardItemModel* resultModel = nullptr;
    QTableView* resultDisplay = nullptr;
    QStatusBar* statusBar = nullptr;
    QLabel* cameraStatusLabel = nullptr;
    QLabel* barcodeStatusLabel = nullptr;
    QTimer* barcodeClearTimer = nullptr;

    ZXing::BarcodeFormat currentBarcodeFormat = ZXing::BarcodeFormat::None;
    bool isEnabledScan = true;

    // concurrency
    QFutureWatcher<FrameResult>* watcher = nullptr;
    QImage currentDisplayedImage;

    bool autoScrollToBottom = true;  // 默认自动滚动
    QCheckBox *autoScrollCheckBox;

        // 翻转控制标志
    bool flipX = false;
    bool flipY = true;  // 默认Y轴翻转修正相机方向

};


