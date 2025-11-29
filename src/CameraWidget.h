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
#include <ZXing/BarcodeFormat.h>
#include "commondef.h"
class QHideEvent;
class QPushButton;
class QMenuBar;
class QLabel;
class QTimer;
class QTableView;
class QStandardItemModel;
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
    void updateFrame(const FrameResult& r);
    void captureLoop();
    void processFrame(cv::Mat& frame, FrameResult& out) const;

private:
    cv::VideoCapture* capture = nullptr;
    std::atomic_bool running{false};
    std::thread captureThread;
    std::future<void> asyncOpenFuture;
    bool cameraStarted = false;
    std::atomic_bool isEnabledScan = true;
    QVBoxLayout* mainLayout = nullptr;
    FrameWidget* frameWidget = nullptr;
    QTableView* resultDisplay;
    QStandardItemModel* resultModel;
    QStatusBar* statusBar = nullptr;
    QMenuBar* menuBar;
    QMenu* cameraMenu;
    int currentCameraIndex = 0;
    QComboBox* barcodeTypeCombo = nullptr;
    ZXing::BarcodeFormat currentBarcodeFormat = ZXing::BarcodeFormat::None;
    QLabel* cameraStatusLabel;
    QLabel* barcodeStatusLabel;
    QTimer* barcodeClearTimer;
};

#endif // CAMERAWIDGET_H
