#include <QHeaderView>
#include "CameraWidget.h"
#include <ZXing/ReadBarcode.h>
#include <QGroupBox>
#include <QMessageBox>
#include <QMetaObject>
#include <QDateTime>
#include <qaction.h>
#include <spdlog/spdlog.h>
#include <QPushButton>
#include <QComboBox>
#include <QCameraInfo>
#include <spdlog/spdlog.h>
#include <QMenuBar>
#include <QToolButton>
#include <magic_enum/magic_enum_format.hpp>
#include <QWidgetAction>
#include <QLabel>
#include <QTimer>
#include <QStandardItemModel>
#include <QTableView>

static const std::vector<std::pair<ZXing::BarcodeFormat, QString>> kBarcodeFormatList {
    { ZXing::BarcodeFormat::Aztec,           "Aztec" },
    { ZXing::BarcodeFormat::Codabar,         "Codabar" },
    { ZXing::BarcodeFormat::Code39,          "Code39" },
    { ZXing::BarcodeFormat::Code93,          "Code93" },
    { ZXing::BarcodeFormat::Code128,         "Code128" },
    { ZXing::BarcodeFormat::DataBar,         "DataBar" },
    { ZXing::BarcodeFormat::DataBarExpanded, "DataBarExpanded" },
    { ZXing::BarcodeFormat::DataMatrix,      "DataMatrix" },
    { ZXing::BarcodeFormat::EAN8,            "EAN8" },
    { ZXing::BarcodeFormat::EAN13,           "EAN13" },
    { ZXing::BarcodeFormat::ITF,             "ITF" },
    { ZXing::BarcodeFormat::MaxiCode,        "MaxiCode" },
    { ZXing::BarcodeFormat::PDF417,          "PDF417" },
    { ZXing::BarcodeFormat::QRCode,          "QRCode" },
    { ZXing::BarcodeFormat::UPCA,            "UPCA" },
    { ZXing::BarcodeFormat::UPCE,            "UPCE" },
    { ZXing::BarcodeFormat::MicroQRCode,     "MicroQRCode" },
    { ZXing::BarcodeFormat::RMQRCode,        "RMQRCode" },
    { ZXing::BarcodeFormat::DXFilmEdge,      "DXFilmEdge" },
    { ZXing::BarcodeFormat::DataBarLimited,  "DataBarLimited" },
};

/**
 * @brief 将 cv::Mat 转换为 ZXing::ImageView
 * @param image 输入的 cv::Mat 图像
 * @return 转换后的 ZXing::ImageView 对象
 */
static ZXing::ImageView ImageViewFromMat(const cv::Mat& image)
{
    using ZXing::ImageFormat;
    auto fmt = ImageFormat::None;
    switch (image.channels()) {
        case 1: fmt = ImageFormat::Lum; break;
        case 3: fmt = ImageFormat::BGR; break;
        case 4: fmt = ImageFormat::BGRA; break;
        default: return { nullptr,0,0,ImageFormat::None };
    }
    if (image.depth() != CV_8U) 
        return {nullptr,0,0,ImageFormat::None};

    return { image.data, image.cols, image.rows, fmt };
}

/**
 * @brief 在图像上绘制条码的边界框和文本
 *
 * @param img 输入输出图像，绘制条形码的边界框和文本
 * @param bc 条码对象，包含条码的位置信息和识别的文本
 */
static void DrawBarcode(cv::Mat& img, const ZXing::Barcode& bc)
{
    const auto pos = bc.position();
    const auto cvp = [](ZXing::PointI p) { return cv::Point(p.x, p.y); };
    const std::vector<cv::Point> pts = {cvp(pos[0]), cvp(pos[1]), cvp(pos[2]), cvp(pos[3])};
    cv::polylines(img, pts, true, CV_RGB(0,255,0));
    cv::putText(img, bc.text(), cvp(pos[3]) + cv::Point(0,20),
                cv::FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(0,255,0));
}

// 构造函数里枚举摄像头
CameraWidget::CameraWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("摄像头预览");
    setMinimumSize(800, 600);

    mainLayout = new QVBoxLayout(this);
    menuBar = new QMenuBar(this);

    cameraMenu = new QMenu("摄像头", this);
    menuBar->addMenu(cameraMenu);
    mainLayout->setMenuBar(menuBar); // 将菜单栏加到窗口布局

    // 菜单栏
    QMenu* scanMenu = menuBar->addMenu("二维码类型");

    // 全选按钮
    QAction* selectAllAction = new QAction("全选", this);
    scanMenu->addAction(selectAllAction);

    // 清空按钮
    QAction* clearAction = new QAction("清空", this);
    scanMenu->addAction(clearAction);

    // 添加分隔符
    scanMenu->addSeparator();

    // 逐个添加格式
    QVector<QAction*> formatActions;
    for (const auto& item : kBarcodeFormatList) {
        ZXing::BarcodeFormat fmt = item.first;
        const QString& name = item.second;

        QAction* act = new QAction(name, this);
        act->setCheckable(true);
        act->setChecked(true);
        act->setData(static_cast<int>(fmt));
        scanMenu->addAction(act);

        formatActions.push_back(act);
    }

    // 全选
    connect(selectAllAction, &QAction::triggered, this, [this,formatActions]{
        for (auto* act : formatActions) act->setChecked(true);
        isEnabledScan = true;
    });

    // 清空
    connect(clearAction, &QAction::triggered, this, [this,formatActions]{
        for (auto* act : formatActions) act->setChecked(false);
        isEnabledScan = false;
    });

    // 更新 currentBarcodeFormat
    auto updateMask = [this,formatActions]{
        bool anyChecked = false;
        ZXing::BarcodeFormat mask = ZXing::BarcodeFormat::None;

        for (const auto* a : formatActions) {
            if (a->isChecked()) {
                anyChecked = true;
                mask = static_cast<ZXing::BarcodeFormat>(
                    static_cast<int>(mask) |
                    a->data().toInt()  
                );
            }
        }

        currentBarcodeFormat = mask;
        isEnabledScan = anyChecked;
    };

    for (const auto* act : formatActions)
        connect(act, &QAction::toggled, this, updateMask);

    const auto cameraDescriptions = CameraConfig::getCameraDescriptions();
    spdlog::info("Available cameras: {}", cameraDescriptions.size());
    for (int i = 0; i < cameraDescriptions.size(); ++i) {
        spdlog::info("Camera {}: {}", i, cameraDescriptions[i].toStdString());
        QAction* action = new QAction(cameraDescriptions[i], this);
        action->setData(i);  // 存摄像头索引
        cameraMenu->addAction(action);
        connect(action, &QAction::triggered, this, [this, action] {
            const int index = action->data().toInt();
            currentCameraIndex = index;  // 更新当前摄像头
            onCameraIndexChanged(index); // 切换摄像头
        });
    }

    // FrameWidget: 可缩放
    frameWidget = new FrameWidget();
    frameWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(frameWidget, 1);

    {
        resultModel = new QStandardItemModel(0, 4, this); // 行，列
        resultModel->setHorizontalHeaderLabels({"时间", "类型", "内容", "状态"});

        resultDisplay = new QTableView(this);
        resultDisplay->setModel(resultModel);
        resultDisplay->setMaximumHeight(150);
        resultDisplay->setSelectionBehavior(QAbstractItemView::SelectRows);
        resultDisplay->setEditTriggers(QAbstractItemView::NoEditTriggers);
        resultDisplay->verticalHeader()->setVisible(false); // 隐藏行号
        // 或者使用比例方式
        resultDisplay->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // 时间固定
        resultDisplay->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // 类型固定
        resultDisplay->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // 内容拉伸

        resultDisplay->setAlternatingRowColors(true);
        mainLayout->addWidget(resultDisplay);
    }
    {
        statusBar = new QStatusBar(this);
        
        // 创建相机状态标签（左对齐）
        cameraStatusLabel = new QLabel("摄像头就绪...", this);
        statusBar->addWidget(cameraStatusLabel); // 默认左对齐
        
        // 添加弹簧将条码状态推到右边
        statusBar->addPermanentWidget(new QLabel("")); // 空标签作为弹簧
        
        // 创建条码状态标签（右对齐）
        barcodeStatusLabel = new QLabel(this);
        barcodeStatusLabel->setAlignment(Qt::AlignRight);
        statusBar->addPermanentWidget(barcodeStatusLabel);
        
        mainLayout->addWidget(statusBar);
        
        // 初始化计时器
        barcodeClearTimer = new QTimer(this);
        barcodeClearTimer->setSingleShot(true);
        connect(barcodeClearTimer, &QTimer::timeout, this, [this]() {
            barcodeStatusLabel->clear();
            barcodeStatusLabel->setStyleSheet("");
        });
    }
    
}

void CameraWidget::onCameraIndexChanged(int index)
{
    if (cameraStarted) {
        // 已启动则切换摄像头
        stopCamera();
        startCamera(index);
    }
}


void CameraWidget::hideEvent(QHideEvent* event)
{
    stopCamera();  // 窗口隐藏时停止摄像头
    QWidget::hideEvent(event);  // 保留基类行为
}

void CameraWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);  // 保留基类行为
    if (!cameraStarted) {
        startCamera(currentCameraIndex); // 使用当前摄像头索引
    }
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
    (void)CameraConfig::getSupportedCameraConfigs(camIndex);
    asyncOpenFuture = std::async(std::launch::async, [this, camIndex] {
        spdlog::info("Opening VideoCapture index {}", camIndex);
        auto cap = std::make_unique<cv::VideoCapture>(camIndex);
        if (!cap->isOpened()) {
            spdlog::error("Failed to open camera {}", camIndex);
            QMetaObject::invokeMethod(this, [this] {
                QMessageBox::warning(this, "错误", "无法打开摄像头");
                cameraStarted = false;
            }, Qt::QueuedConnection);
            return;
        }
        const auto config = CameraConfig::selectBestCameraConfig(CameraConfig::getSupportedCameraConfigs(camIndex));
        spdlog::info("Selected Camera Config - Resolution: {}x{}, FPS: {}, Pixel Format: {}",
            config.width, config.height, config.fps, config.pixelFormat.toStdString());

        cap->set(cv::CAP_PROP_FRAME_WIDTH, config.width);
        cap->set(cv::CAP_PROP_FRAME_HEIGHT, config.height);
        cap->set(cv::CAP_PROP_FPS, config.fps);

        this->capture = cap.release();

        QMetaObject::invokeMethod(this, [this] {
            cameraStatusLabel->setText("摄像头已启动");
            captureThread = std::thread(&CameraWidget::captureLoop, this);
        }, Qt::QueuedConnection);
    });
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

    cameraStatusLabel->setText("摄像头已停止");
}

void CameraWidget::updateFrame(const FrameResult& r) const
{
    // 显示视频帧
    frameWidget->setFrame(r.frame);

    cameraStatusLabel->setText("摄像头运行中...");

    if (r.hasBarcode) {
        barcodeStatusLabel->setText("检测到 " + r.type + " 码");
        barcodeStatusLabel->setStyleSheet("color: green; font-weight: bold;");

        QString resultText = QString("条码类型: %1\n内容: %2\n时间: %3\n------------------------\n")
            .arg(r.type)
            .arg(r.content)
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

        // 检查是否与上一条记录相同
        static QString lastContent;
        static QString lastType;

        if (r.content == lastContent && r.type == lastType) {
            barcodeClearTimer->start(3000);
            return;
        }

        // 更新上一次的记录
        lastContent = r.content;
        lastType = r.type;


        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QDateTime::currentDateTime().toString("hh:mm:ss"));
        rowItems << new QStandardItem(r.type);
        rowItems << new QStandardItem(r.content);

        // 设置颜色
        rowItems[1]->setForeground(Qt::blue); // 类型蓝色
        resultModel->insertRow(0, rowItems); // 插入到顶部

        // 限制行数
        if (resultModel->rowCount() > 50) {
            resultModel->removeRow(50);
        }
        // 导出到Excel
        // ExcelExporter::instance().append(r);
        barcodeClearTimer->start(3000);
    }
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

        QMetaObject::invokeMethod(this, [this, result] { updateFrame(result); }, Qt::QueuedConnection);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    spdlog::info("Capture thread stopped");
}

void CameraWidget::processFrame(cv::Mat& frame, FrameResult& out) const
{
    if(!isEnabledScan) return;
    const auto barcodes = ZXing::ReadBarcodes(ImageViewFromMat(frame));
    for (auto& bc : barcodes) {
        if (!bc.isValid()) continue;

        // 如果 currentBarcodeFormat = None -> 全部模式
        if (currentBarcodeFormat != ZXing::BarcodeFormat::None &&
            !(static_cast<int>(bc.format()) & static_cast<int>(currentBarcodeFormat))) {
            continue; // 当前格式未被选中，跳过
        }

        out.hasBarcode = true;
        out.type = QString::fromStdString(ZXing::ToString(bc.format()));
        out.content = QString::fromStdString(bc.text());
        DrawBarcode(frame, bc);
    }
}
