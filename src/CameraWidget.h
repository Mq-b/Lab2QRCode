#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <future>
#include <atomic>
#include <qcombobox.h>
#include <thread>
#include <opencv2/opencv.hpp>
#include <QWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include "FrameWidget.h"
#include <zxing/BarcodeFormat.h>
struct FrameResult
{
    cv::Mat frame; 
    bool hasBarcode = false;
    QString type;
    QString content;
};
class QHideEvent;
class QPushButton;
class QMenuBar;
class CameraWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CameraWidget(QWidget* parent = nullptr);
    ~CameraWidget();
    
    void startCamera(int camIndex = 0);
    void stopCamera();
protected:
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void onCameraIndexChanged(int index);
    void toggleCamera();
    void updateFrame(const FrameResult& r);
    void captureLoop();
    void processFrame(cv::Mat& frame, FrameResult& out) const;
    void displayScanResult(const std::string& type, const std::string& content) const;

private:
    cv::VideoCapture* capture = nullptr;
    std::atomic_bool running{false};
    std::thread captureThread;
    std::future<void> asyncOpenFuture;
    bool cameraStarted = false;
    std::atomic_bool isEnabledScan = true;
    QVBoxLayout* mainLayout = nullptr;
    FrameWidget* frameWidget = nullptr;
    QTextEdit* resultDisplay = nullptr;
    QStatusBar* statusBar = nullptr;
    QMenuBar* menuBar;
    QMenu* cameraMenu;
    int currentCameraIndex = 0;
    QComboBox* barcodeTypeCombo = nullptr;
    ZXing::BarcodeFormat currentBarcodeFormat = ZXing::BarcodeFormat::None; // 默认检测所有
};

#endif // CAMERAWIDGET_H
