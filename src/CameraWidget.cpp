#include "CameraWidget.h"
#include <ZXing/ReadBarcode.h>
#include <QGroupBox>
#include <QMessageBox>
#include <QMetaObject>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <QPushButton>
#include <QComboBox>
#include <QCameraInfo>
#include <spdlog/spdlog.h>
#include <QCameraInfo>

// 构造函数里枚举摄像头
CameraWidget::CameraWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("摄像头预览");
    setMinimumSize(800, 600);

    mainLayout = new QVBoxLayout(this);

    auto* topLayout = new QHBoxLayout();
    cameraCombo = new QComboBox(this);
    topLayout->addWidget(cameraCombo);

    mainLayout->addLayout(topLayout);

    // 使用 QCameraInfo 获取可用摄像头，避免 cv::VideoCapture 测试设备卡住
    const auto cameras = QCameraInfo::availableCameras();
    for (int i = 0; i < cameras.size(); ++i) {
        const auto& camInfo = cameras[i];
        cameraCombo->addItem(
            QString("%1 [%2]").arg(camInfo.description()).arg(i),
            i
        );
        spdlog::info("camInfo: {}",camInfo.description().toStdString());
        cameraCombo->setItemData(i, camInfo.deviceName(), Qt::ToolTipRole);
    }
    spdlog::info("availableCameras: {}",cameras.size());
    connect(cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CameraWidget::onCameraIndexChanged);
    // FrameWidget: 可缩放
    frameWidget = new FrameWidget();
    frameWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(frameWidget, 1);

    // 扫码结果
    auto* resultGroup = new QGroupBox("扫码结果", this);
    auto* resultLayout = new QVBoxLayout(resultGroup);
    resultDisplay = new QTextEdit(this);
    resultDisplay->setReadOnly(true);
    resultDisplay->setMaximumHeight(120);
    resultLayout->addWidget(resultDisplay);
    resultGroup->setLayout(resultLayout);
    mainLayout->addWidget(resultGroup);

    // 状态栏
    statusBar = new QStatusBar(this);
    statusBar->showMessage("摄像头就绪...");
    mainLayout->addWidget(statusBar);
}

void CameraWidget::onCameraIndexChanged(int index)
{
    if (cameraStarted) {
        // 已启动则切换摄像头
        stopCamera();
        startCamera(index);
    }
}
void CameraWidget::toggleCamera()
{
    if (cameraStarted) stopCamera();
    else startCamera(cameraCombo->currentIndex());
}
void CameraWidget::hideEvent(QHideEvent* event)
{
    stopCamera();  // 窗口隐藏时停止摄像头
    QWidget::hideEvent(event);  // 保留基类行为
}
CameraWidget::~CameraWidget()
{
    stopCamera();
}
// 启动摄像头（可指定索引）
void CameraWidget::startCamera(int camIndex)
{
    if (cameraStarted) return;

    cameraStarted = true;
    running = true;
    asyncOpenFuture = std::async(std::launch::async, [this, camIndex]() {
        spdlog::info("Opening VideoCapture index {}", camIndex);
        auto cap = std::make_unique<cv::VideoCapture>(camIndex, cv::CAP_DSHOW); // Windows 可用 DSHOW
        if (!cap->isOpened()) {
            spdlog::error("Failed to open camera {}", camIndex);
            QMetaObject::invokeMethod(this, [this]() {
                QMessageBox::warning(this, "错误", "无法打开摄像头");
                cameraStarted = false;
            }, Qt::QueuedConnection);
            return;
        }

        cap->set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap->set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        cap->set(cv::CAP_PROP_FPS, 30);

        this->capture = cap.release();

        QMetaObject::invokeMethod(this, [this]() {
            statusBar->showMessage("摄像头已启动");
            captureThread = std::thread(&CameraWidget::captureLoop, this);
        }, Qt::QueuedConnection);
    });
}
void CameraWidget::captureLoop()
{
    spdlog::info("Capture thread started");
    while (running) {
        FrameResult result;
        cv::Mat frame;
        *capture >> frame;

        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

        processFrame(frame, result);
        result.frame = frame;

        QMetaObject::invokeMethod(this, [this, result]() { updateFrame(result); }, Qt::QueuedConnection);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    spdlog::info("Capture thread stopped");
}

void CameraWidget::stopCamera()
{
    frameWidget->clear();
    if (!cameraStarted) return;
    running = false;
    if (captureThread.joinable()) captureThread.join();

    if (capture) {
        if (capture->isOpened()) capture->release();
        delete capture;
    }
    capture = nullptr;
    cameraStarted = false;

    statusBar->showMessage("摄像头已停止");
}

void CameraWidget::updateFrame(const FrameResult& r)
{
    frameWidget->setFrame(r.frame);

    if (r.hasBarcode) {
        statusBar->showMessage("检测到 " + r.type + " 码");
        displayScanResult(r.type.toStdString(), r.content.toStdString());
    } else {
        statusBar->showMessage("摄像头运行中...");
    }
}

// ----- Frame + ZXing -----
inline ZXing::ImageView ImageViewFromMat(const cv::Mat& image)
{
    using ZXing::ImageFormat;
    ImageFormat fmt = ImageFormat::None;
    switch (image.channels()) {
    case 1: fmt = ImageFormat::Lum; break;
    case 3: fmt = ImageFormat::BGR; break;
    case 4: fmt = ImageFormat::BGRA; break;
    }
    if (image.depth() != CV_8U) return {nullptr,0,0,ImageFormat::None};
    return { image.data, image.cols, image.rows, fmt };
}

inline void DrawBarcode(cv::Mat& img, ZXing::Barcode bc)
{
    auto pos = bc.position();
    auto cvp = [](ZXing::PointI p) { return cv::Point(p.x, p.y); };
    std::vector<cv::Point> pts = {cvp(pos[0]), cvp(pos[1]), cvp(pos[2]), cvp(pos[3])};
    cv::polylines(img, pts, true, CV_RGB(0,255,0));
    cv::putText(img, bc.text(), cvp(pos[3]) + cv::Point(0,20),
                cv::FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(0,255,0));
}

void CameraWidget::processFrame(cv::Mat& frame, FrameResult& out) const
{
    auto barcodes = ZXing::ReadBarcodes(ImageViewFromMat(frame));
    for (auto& bc : barcodes) {
        if (!bc.isValid()) continue;
        out.hasBarcode = true;
        out.type = QString::fromStdString(ZXing::ToString(bc.format()));
        out.content = QString::fromStdString(bc.text());
        DrawBarcode(frame, bc);
    }
}

void CameraWidget::displayScanResult(const std::string& type, const std::string& content) const
{
    QString t = QString("条码类型: %1\n内容: %2\n时间: %3\n------------------------\n")
                .arg(QString::fromStdString(type))
                .arg(QString::fromStdString(content))
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    resultDisplay->insertPlainText(t);
}
