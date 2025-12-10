#include <QHeaderView>
#include "CameraWidget.h"
#include <ZXing/ReadBarcode.h>
#include <QGroupBox>
#include <QMessageBox>
#include <QMetaObject>
#include <QDateTime>
#include <qaction.h>
#include <qcoreevent.h>
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
#include <QBuffer>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>
#include <QToolButton>
#include <xlsxwriter.h>

static const auto HISTOGRAM_CLIP_THRESHOLD = 0.1;

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

/**
 * @brief 将多边形区域修正为矩形图片
 *        为了不裁剪到条码，增加了一定的边距
 * @param img 原始图像
 * @param bc 条码对象，包含条码的位置信息和识别的文本
 * @param enhance 是否对结果进行图像增强
 *                如果为 true，则对修正后的图像进行增强处理（对比度拉伸与亮度非线性映射），以提高条码的可读性。
 * @return 修正后的矩形图片
 */
cv::Mat RectifyPolygonToRect(const cv::Mat& img, const ZXing::Barcode& bc, bool enhance)
{
    const auto corners = bc.position();
    const std::vector<cv::Point2f> barcodeCorners = {
        cv::Point2f(corners[0].x, corners[0].y),
        cv::Point2f(corners[1].x, corners[1].y),
        cv::Point2f(corners[2].x, corners[2].y),
        cv::Point2f(corners[3].x, corners[3].y)
    };
    const auto calcDistance = [](const cv::Point2f& a, const cv::Point2f& b) {
        cv::Point2f diff = a - b;
        return std::sqrt(diff.x * diff.x + diff.y * diff.y);
    };
    const auto maxSideLength = std::max({
        calcDistance(barcodeCorners[0], barcodeCorners[1]),
        calcDistance(barcodeCorners[1], barcodeCorners[2]),
        calcDistance(barcodeCorners[2], barcodeCorners[3]),
        calcDistance(barcodeCorners[3], barcodeCorners[0]),
    });
    const std::vector<cv::Point2f> rectCorners = {
        cv::Point2f(0, 0),
        cv::Point2f(maxSideLength - 1, 0),
        cv::Point2f(maxSideLength - 1, maxSideLength - 1),
        cv::Point2f(0, maxSideLength - 1)
    };
    const std::vector<cv::Point2f> rectCornersWithMargin = {
        cv::Point2f(-0.05 * maxSideLength, -0.05 * maxSideLength),
        cv::Point2f(1.05 * maxSideLength - 1, -0.05 * maxSideLength),
        cv::Point2f(1.05 * maxSideLength - 1, 1.05 * maxSideLength - 1),
        cv::Point2f(-0.05 * maxSideLength, 1.05 * maxSideLength - 1)
    };
    const auto outputSize = static_cast<int>(maxSideLength * 1.1);
    const std::vector<cv::Point2f> outputRect = {
        cv::Point2f(0, 0),
        cv::Point2f(outputSize - 1, 0),
        cv::Point2f(outputSize - 1, outputSize - 1),
        cv::Point2f(0, outputSize - 1)
    };
    cv::Mat toBarcodeTransform = cv::getPerspectiveTransform(rectCorners, barcodeCorners);
    std::vector<cv::Point2f> marginBarcodeCorners(4);
    cv::perspectiveTransform(rectCornersWithMargin, marginBarcodeCorners, toBarcodeTransform);
    cv::Mat toOutputRectTransform = cv::getPerspectiveTransform(marginBarcodeCorners, outputRect);
    cv::Mat rectifiedImage;
    cv::warpPerspective(img, rectifiedImage, toOutputRectTransform, cv::Size(outputSize, outputSize));
    if (!enhance) {
        return rectifiedImage;
    }

    if (rectifiedImage.channels() == 1) {
        cv::cvtColor(rectifiedImage, rectifiedImage, cv::COLOR_GRAY2BGR);
    } else if (rectifiedImage.channels() == 4) {
        cv::cvtColor(rectifiedImage, rectifiedImage, cv::COLOR_BGRA2BGR);
    } else if (rectifiedImage.channels() != 3) {
        return rectifiedImage; // 不支持的通道数，直接返回原图
    }

    std::vector<int> bCount(256, 0), gCount(256, 0), rCount(256, 0);
    for (int y = 0; y < rectifiedImage.rows; y++) {
        const auto row = rectifiedImage.ptr<cv::Vec3b>(y);
        for (int x = 0; x < rectifiedImage.cols; x++) {
            bCount[row[x][0]]++;
            gCount[row[x][1]]++;
            rCount[row[x][2]]++;
        }
    }

    const auto calc_lo_hi = [](const std::vector<int>& count) -> std::pair<int, int> {
        int total = 0;
        for (int v = 0; v < 256; v++) {
            total += count[v];
        }
        int lo = 0, lo_sum = 0;
        for (int v = 0; v < 256; v++) {
            lo_sum += count[v];
            if (lo_sum >= total * HISTOGRAM_CLIP_THRESHOLD) {
                lo = v;
                break;
            }
        }
        int hi = 255, hi_sum = 0;
        for (int v = 255; v >= 0; v--) {
            hi_sum += count[v];
            if (hi_sum >= total * HISTOGRAM_CLIP_THRESHOLD) {
                hi = v;
                break;
            }
        }
        return {lo, hi};
    };
    const auto [bLow, bHigh] = calc_lo_hi(bCount);
    const auto [gLow, gHigh] = calc_lo_hi(gCount);
    const auto [rLow, rHigh] = calc_lo_hi(rCount);

    cv::Mat fimg;
    rectifiedImage.convertTo(fimg, CV_32F);
    {
        std::vector<cv::Mat> channels(3);
        cv::split(fimg, channels);
        const auto norm_clip = [](cv::Mat& ch, int lo, int hi) {
            ch = (ch - lo) / (hi - lo);
            cv::min(ch, 1.0, ch);
            cv::max(ch, 0.0, ch);
        };
        norm_clip(channels[0], bLow, bHigh);
        norm_clip(channels[1], gLow, gHigh);
        norm_clip(channels[2], rLow, rHigh);
        cv::merge(channels, fimg);
    }

    cv::Mat mat1;
    cv::pow(fimg, 2, mat1);
    cv::Mat mat2;
    cv::pow(fimg, 3, mat2);
    cv::Mat enhanced;
    cv::Mat(3 * mat1 - 2 * mat2).convertTo(enhanced, CV_8U, 255.0);

    return enhanced;
}

// 构造函数里枚举摄像头
CameraWidget::CameraWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("摄像头预览");
    setMinimumSize(800, 600);
    this->installEventFilter(this);

    mainLayout = new QVBoxLayout(this);
    menuBar = new QMenuBar(this);

    cameraMenu = new QMenu("摄像头", this);
    menuBar->addMenu(cameraMenu);

    cameraConfigMenu = new QMenu("显示设置", this);
    menuBar->addMenu(cameraConfigMenu);

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

    QMenu* postProcessingMenu = menuBar->addMenu("后处理");

    QAction* enhanceAction = new QAction("图像增强", this);
    enhanceAction->setCheckable(true);
    enhanceAction->setChecked(isEnhanceEnabled);
    postProcessingMenu->addAction(enhanceAction);

    connect(enhanceAction, &QAction::toggled, this, [this](bool checked) {
        isEnhanceEnabled = checked;
    });

    // FrameWidget: 可缩放
    frameWidget = new FrameWidget();
    frameWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(frameWidget, 1);

    {
        resultModel = new QStandardItemModel(0, 7, this); // 行，列
        resultModel->setHorizontalHeaderLabels({"时间", "图像", "类型", "内容", "[隐藏] PNG 数据", "[隐藏] 图片宽度", "[隐藏] 图片高度"});

        resultDisplay = new QTableView(this);
        resultDisplay->setModel(resultModel);
        resultDisplay->setMaximumHeight(150);
        resultDisplay->setSelectionBehavior(QAbstractItemView::SelectRows);
        resultDisplay->setEditTriggers(QAbstractItemView::NoEditTriggers);
        resultDisplay->verticalHeader()->setVisible(false); // 隐藏行号
        resultDisplay->setColumnHidden(4, true); // 隐藏 PNG 数据列
        resultDisplay->setColumnHidden(5, true); // 隐藏 图片宽度 列
        resultDisplay->setColumnHidden(6, true); // 隐藏 图片高度 列
        // 或者使用比例方式
        resultDisplay->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // 时间固定
        resultDisplay->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // 图像固定
        resultDisplay->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed); // 类型固定
        resultDisplay->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch); // 内容拉伸

        resultDisplay->setAlternatingRowColors(true);
        resultDisplay->installEventFilter(this);
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

        // 导出按钮（HTML / XLSX）
        QToolButton* exportButton = new QToolButton(this);
        exportButton->setText("导出");
        QMenu* exportMenu = new QMenu(exportButton);
        QAction* exportHtmlAction = new QAction("导出 HTML (.html)", exportMenu);
        QAction* exportXlsxAction = new QAction("导出 XLSX (.xlsx)", exportMenu);
        exportMenu->addAction(exportHtmlAction);
        exportMenu->addAction(exportXlsxAction);
        exportButton->setMenu(exportMenu);
        exportButton->setPopupMode(QToolButton::InstantPopup);
        statusBar->addPermanentWidget(exportButton);

        connect(exportHtmlAction, &QAction::triggered, this, [this]() {
            const QString def = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "scan_results.html";
            const QString path = QFileDialog::getSaveFileName(this, "保存为 HTML (.html)", def, "HTML 文件 (*.html)");
            if (!path.isEmpty()) {
                if (exportResultsToHtml(path)) {
                    QMessageBox::information(this, "导出完成", "已导出 HTML 文件：\n" + path);
                } else {
                    QMessageBox::warning(this, "导出失败", "导出 HTML 文件失败：\n" + path);
                }
            }
        });

        connect(exportXlsxAction, &QAction::triggered, this, [this]() {
            const QString def = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "scan_results.xlsx";
            const QString path = QFileDialog::getSaveFileName(this, "保存为 XLSX (.xlsx)", def, "Excel 文件 (*.xlsx)");
            if (!path.isEmpty()) {
                if (exportResultsToXlsx(path)) {
                    QMessageBox::information(this, "导出完成", "已导出 XLSX 文件：\n" + path);
                } else {
                    QMessageBox::warning(this, "导出失败", "导出 XLSX 文件失败：\n" + path);
                }
            }
        });
    }
    
}

bool CameraWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == resultDisplay && event->type() == QEvent::FocusOut) {
        resultDisplay->clearSelection();
    }
    if (obj != resultDisplay && event->type() == QEvent::MouseButtonPress) {
        resultDisplay->clearFocus();
    }
    return QWidget::eventFilter(obj, event);
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
        // 根据打开的摄像头加载摄像头配置
        std::vector<CameraConfig> configs = CameraConfig::getSupportedCameraConfigs(camIndex);
        
        // 回到主线程，创建菜单 UI
        QMetaObject::invokeMethod(this,
            [this, configs]() { loadCameraConfigs(configs); },
            Qt::QueuedConnection);
        // 默认选择最佳配置
        const auto config = CameraConfig::selectBestCameraConfig(configs);
        spdlog::info("Selected Camera Config - Resolution: {}x{}, FPS: {}, Pixel Format: {}",
            config.width, config.height, config.fps, config.pixelFormat.toStdString());
        //勾选对应的配置，在主线程操作 UI
        QMetaObject::invokeMethod(this, [this, config]() {selectBestCameraConfigUI(config);},
            Qt::QueuedConnection);
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
        QStandardItem* imageItem = new QStandardItem();
        if (r.rectifiedImage.empty()) {
            // If the rectified image is empty, skip adding this result
            return;
        }
        QImage img = QImage((const uchar*)r.rectifiedImage.data, r.rectifiedImage.cols, r.rectifiedImage.rows, r.rectifiedImage.step, QImage::Format_RGB888).rgbSwapped();
        QPixmap pixmap = QPixmap::fromImage(img).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageItem->setData(pixmap, Qt::DecorationRole);
        rowItems << imageItem;
        rowItems << new QStandardItem(r.type);
        rowItems << new QStandardItem(r.content);
        // 存储 PNG 数据以便导出
        QByteArray pngData;
        QBuffer buffer(&pngData);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");
        buffer.close();
        rowItems << new QStandardItem(QString::fromLatin1(pngData.toBase64()));
        rowItems << new QStandardItem(QString::number(img.width())); // 图片宽度
        rowItems << new QStandardItem(QString::number(img.height())); // 图片高度

        // 设置颜色
        rowItems[2]->setForeground(Qt::blue); // 类型蓝色
        resultModel->insertRow(0, rowItems); // 插入到顶部
        resultDisplay->setRowHeight(0, 128); // 设置行高
        resultDisplay->setColumnWidth(1, 128); // 设置图片列宽度

        // 限制行数
        if (resultModel->rowCount() > 50) {
            resultModel->removeRow(50);
        }
        barcodeClearTimer->start(3000);
    }
}

bool CameraWidget::exportResultsToHtml(const QString& filePath)
{
    QString html;
    html.reserve(4096);
    html += "<!DOCTYPE html>\n";
    html += "<html lang=zh-CN><meta charset=UTF-8><meta content='width=device-width,initial-scale=1'name=viewport><title>扫描结果</title><style>body{font-family:'Segoe UI',Aria";
    html += "l,'Microsoft YaHei',sans-serif;background:#f7f7f7;margin:0;padding:0;display:flex;justify-content:center;align-items:flex-start;min-height:100vh}table{border-colla";
    html += "pse:separate;border-spacing:0;width:800px;background:#fff;box-shadow:0 2px 12px rgba(0,0,0,.08);border-radius:12px;overflow:hidden;margin:40px auto}caption{font-si";
    html += "ze:1.5em;font-weight:700;padding:18px 0 10px 0;color:#333;background:#f0f4fa;border-bottom:1px solid #eaeaea}thead th{background:#f0f4fa;color:#333;font-weight:600";
    html += ";padding:14px 10px;border-bottom:2px solid #eaeaea;text-align:center}tbody td{padding:12px 10px;border-bottom:1px solid #f0f0f0;text-align:center;color:#444}tbody ";
    html += "tr:last-child td{border-bottom:none}tbody tr{transition:background .2s}tbody tr:hover{background:#eaf6ff}tbody img{max-width:60px;max-height:60px;border-radius:6px";
    html += ";box-shadow:0 1px 4px rgba(0,0,0,.07);background:#fafafa;border:1px solid #eaeaea}#preview{position:fixed;display:none;z-index:9999;border:2px solid #eaeaea;backgr";
    html += "ound:#fff;box-shadow:0 4px 24px rgba(0,0,0,.18);border-radius:10px;overflow:hidden}#preview img{max-width:400px;max-height:400px;display:block}</style><table><capt";
    html += "ion>扫描结果<thead><tr><th>时间<th>图像<th>类型<th>内容<tbody>";

    for (int r = 0; r < resultModel->rowCount(); r++) {
        const auto getText = [&](int c) -> QString {
            QStandardItem* it = resultModel->item(r, c);
            QString text = it ? it->text() : QString("");
            text.replace("&", "&amp;");
            text.replace("<", "&lt;");
            text.replace(">", "&gt;");
            text.replace("\"", "&quot;");
            text.replace("'", "&#39;");
            return text;
        };
        html += "<tr>";
        html += "<td>" + getText(0) + "</td>";
        html += "<td>";
        if (QStandardItem* imgItem = resultModel->item(r, 4); imgItem) {
            html += "<img src=\"data:image/png;base64," + imgItem->text() + "\" width=\"128\" />";
        }
        html += "</td>";
        html += "<td>" + getText(2) + "</td>";
        html += "<td>" + getText(3) + "</td>";
        html += "</tr>";
    }

    html += "</tbody></table><div id='preview'><img /></div><script>const preview=document.getElementById('preview');document.querySelectorAll('tbody img').forEach(e=>{e.addEvent";
    html += "Listener('mouseenter',function(t){let i=e.getAttribute('src'),l=preview.querySelector('img');l.src=i,preview.style.display='block',preview.style.left=t.clientX+20+'p";
    html += "x',preview.style.top=t.clientY+'px'}),e.addEventListener('mousemove',function(e){preview.style.left=e.clientX+20+'px',preview.style.top=e.clientY+'px'}),e.addEventLi";
    html += "stener('mouseleave',function(){preview.style.display='none'})});</script></body></html>";

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        spdlog::error("Failed to open export file {}", filePath.toStdString());
        return false;
    }
    f.write(html.toUtf8());
    f.close();
    spdlog::info("Exported scan results to {}", filePath.toStdString());
    return true;
}

bool CameraWidget::exportResultsToXlsx(const QString& filePath)
{
    lxw_workbook  *workbook  = workbook_new(filePath.toStdString().c_str());
    if (!workbook) {
        spdlog::error("Failed to create workbook {}", filePath.toStdString());
        return false;
    }
    lxw_format *center_format = workbook_add_format(workbook);
    format_set_align(center_format, LXW_ALIGN_CENTER);
    format_set_align(center_format, LXW_ALIGN_VERTICAL_CENTER);
    format_set_shrink(center_format);
    lxw_format *center_wrap_format = workbook_add_format(workbook);
    format_set_align(center_wrap_format, LXW_ALIGN_CENTER);
    format_set_align(center_wrap_format, LXW_ALIGN_VERTICAL_CENTER);
    format_set_text_wrap(center_wrap_format);
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, "扫描结果");

    worksheet_set_column(worksheet, 0, 0, 9, center_format);
    worksheet_set_column_pixels(worksheet, 1, 1, 150, center_format);
    worksheet_set_column(worksheet, 2, 2, 9, center_format);
    worksheet_set_column(worksheet, 3, 3, 120, center_wrap_format);

    worksheet_write_string(worksheet, 0, 0, "时间", center_format);
    worksheet_write_string(worksheet, 0, 1, "图像", center_format);
    worksheet_write_string(worksheet, 0, 2, "类型", center_format);
    worksheet_write_string(worksheet, 0, 3, "内容", center_format);

    for (int r = 0; r < resultModel->rowCount(); r++) {
        const int row = r + 1;
        const auto getText = [&](int c)->std::string {
            QStandardItem* it = resultModel->item(r, c);
            return it ? it->text().toStdString() : std::string();
        };

        worksheet_set_row_pixels(worksheet, row, 150, NULL);

        const auto t0 = getText(0);
        const auto t1 = getText(2);
        const auto t2 = getText(3);

        worksheet_write_string(worksheet, row, 0, t0.c_str(), NULL);
        worksheet_write_string(worksheet, row, 2, t1.c_str(), NULL);
        worksheet_write_string(worksheet, row, 3, t2.c_str(), NULL);

        if (QStandardItem* imgItem = resultModel->item(r, 4); imgItem) {
            QString base64Data = imgItem->text();
            QByteArray ba = QByteArray::fromBase64(base64Data.toLatin1());
            const int imgWidth = std::stoi(getText(5));
            const int imgHeight = std::stoi(getText(6));
            lxw_image_options options = {
                .x_scale = 150.0 / imgWidth,
                .y_scale = 150.0 / imgHeight,
            };
            worksheet_insert_image_buffer_opt(worksheet, row, 1, (const unsigned char *)ba.constData(), ba.size(), &options);
        }
    }

    workbook_close(workbook);
    spdlog::info("Exported XLSX to {}", filePath.toStdString());
    return true;
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
        out.rectifiedImage = RectifyPolygonToRect(frame, bc, isEnhanceEnabled);
        DrawBarcode(frame, bc);
    }
}

void CameraWidget::onCameraConfigSelected(CameraConfig config) {
    if (!capture) {
        spdlog::error("Failed to open camera {}", currentCameraIndex);
        return;
    }
    spdlog::info("Selected Camera Config - Resolution: {}x{}, FPS: {}, Pixel Format: {}",
        config.width, config.height, config.fps, config.pixelFormat.toStdString());
    
    bool okWidth = capture->set(cv::CAP_PROP_FRAME_WIDTH, config.width);
    bool okHeight = capture->set(cv::CAP_PROP_FRAME_HEIGHT, config.height);
    bool okFps = capture->set(cv::CAP_PROP_FPS, config.fps);

    spdlog::info("Set width={}, ok={}", config.width, okWidth);
    spdlog::info("Set height={}, ok={}", config.height, okHeight);
    spdlog::info("Set fps={}, ok={}", config.fps, okFps);

    double actual_width = capture->get(cv::CAP_PROP_FRAME_WIDTH);
    double actual_height = capture->get(cv::CAP_PROP_FRAME_HEIGHT);
    double actual_fps = capture->get(cv::CAP_PROP_FPS);
    spdlog::info("Actual Camera Config - Resolution: {}x{}, FPS: {}",
        actual_width, actual_height, actual_fps);
}


void CameraWidget::loadCameraConfigs(const std::vector<CameraConfig>& configs) {
    // 清空菜单
    cameraConfigMenu->clear();

    // 清理旧的 QActionGroup
    if (cameraActionGroup) {
        delete cameraActionGroup;
        cameraActionGroup = nullptr;
    }
    // 加入摄像头配置 actions
    cameraActionGroup = new QActionGroup(this);
    cameraActionGroup->setExclusive(true);  // 互斥

    for (std::size_t i = 0;i < configs.size(); ++i) {
        QString configText = static_cast<QString>(configs[i]);
        QAction* action = new QAction(configText, this);
        action->setCheckable(true);
        action->setData(static_cast<int>(i));

        cameraActionGroup->addAction(action);
        cameraConfigMenu->addAction(action);

        auto config = configs[i];
        connect(action, &QAction::triggered, this, [this, action, config]() {
            onCameraConfigSelected(config);
            action->setChecked(true);
        });
    }
}

void CameraWidget::selectBestCameraConfigUI(const CameraConfig& bestConfig) const
{
    if (!cameraActionGroup) return;

    // 遍历 group 中的所有 action
    const QString bestConfigText = static_cast<QString>(bestConfig);
    for (QAction* action : cameraActionGroup->actions())
    {
        QString text = action->text();

        if(text == bestConfigText)
        {
            // 打印输出选择的配置
            spdlog::info("Selected Camera Config - {}", text.toStdString());
            action->setChecked(true); // 勾选
            break; // 找到后就退出
        }
    }
}