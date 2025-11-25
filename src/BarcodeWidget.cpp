#include "BarcodeWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <SimpleBase64.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/BitMatrix.h>
#include <ZXing/DecodeHints.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/TextUtfEncoding.h>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include "about_dialog.h"
#include <ranges>
#include "convert.h"
#include "version_info/version.h"

template <typename Ret, typename... Fs>
    requires(std::is_void_v<Ret> || std::is_default_constructible_v<Ret>)
struct overload_def_noop : private Fs... {
    template <typename V, typename... Args>
    constexpr explicit(false) overload_def_noop(std::in_place_type_t<V>, Args&&... fs) :
        Fs{std::forward<Args>(fs)}... {}

    using Fs::operator()...;

    template <typename... T>
        requires(!std::invocable<Fs, T...> && ...)
    Ret operator()(T&&...) const noexcept {
        if constexpr (std::is_void_v<Ret>) {
            return;
        } else {
            return Ret{};
        }
    }
};

template <typename V, typename... Fs>
overload_def_noop(std::in_place_type_t<V>, Fs&&...) -> overload_def_noop<V, std::decay_t<Fs>...>;

void drawIcon(QPainter& p, bool isImage, bool isText){
    if(isImage) {
        // --- 绘制二维码样式图标 (Decode) ---
        // 颜色定义
        QColor darkColor("#333");
        p.setPen(Qt::NoPen);
        p.setBrush(darkColor);

        // 绘制三个定位点 (回字纹)
        // 左上
        p.drawRect(2, 2, 7, 7);
        // 右上
        p.drawRect(15, 2, 7, 7);
        // 左下
        p.drawRect(2, 15, 7, 7);

        // 填充内部白色，形成空心效果
        p.setBrush(Qt::white);
        p.drawRect(3, 3, 5, 5);
        p.drawRect(16, 3, 5, 5);
        p.drawRect(3, 16, 5, 5);

        // 绘制定位点中心实心块
        p.setBrush(darkColor);
        p.drawRect(4, 4, 3, 3);
        p.drawRect(17, 4, 3, 3);
        p.drawRect(4, 17, 3, 3);

        // 绘制中间的一点随机数据块
        p.drawRect(11, 11, 4, 4);
    } else {
        // --- 绘制文本文档样式图标 (Generate) ---
        QColor paperColor(isText ? "#fafafa" : "#eee");
        QColor lineColor("#999");

        // 绘制纸张轮廓 (带右上角折角)
        QPolygon poly;
        poly << QPoint(4, 2) << QPoint(15, 2) << QPoint(20, 7) << QPoint(20, 22) << QPoint(4, 22);

        p.setPen(QPen(lineColor, 1));
        p.setBrush(paperColor);
        p.drawPolygon(poly);

        // 绘制折角线条
        p.drawLine(15, 2, 15, 7);
        p.drawLine(15, 7, 20, 7);

        // 绘制文本横线示意
        p.setPen(QPen(QColor("#bbb"), 1));
        p.drawLine(7, 10, 17, 10);
        p.drawLine(7, 13, 17, 13);
        p.drawLine(7, 16, 14, 16); // 最后一行短一点
    }
}

static QRegularExpression fileExtensionRegex_text(
    R"(^.*\.(?:txt|json|rfa)$)",
    QRegularExpression::CaseInsensitiveOption
);

static QRegularExpression fileExtensionRegex_image(
    R"(^.*\.(?:png|jpg|jpeg|bmp|gif|tiff|webp)$)",
    QRegularExpression::CaseInsensitiveOption
);

BarcodeWidget::BarcodeWidget(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Lab2QRCode");
    setMinimumSize(500, 600);

    QMenuBar* menuBar  = new QMenuBar();
    QMenu*    helpMenu = menuBar->addMenu("帮助");
    QMenu*    toolsMenu = menuBar->addMenu("工具");

    QFont menuFont("SimHei", 12);
    menuBar->setFont(menuFont);

    QAction* aboutAction = new QAction("关于软件", this);
    QAction* debugMqttAction = new QAction("MQTT实时消息监控窗口", this);
    aboutAction->setFont(menuFont);
    helpMenu->addAction(aboutAction);
    toolsMenu->addAction(debugMqttAction);

    // 连接菜单项的点击信号
    connect(aboutAction, &QAction::triggered, this, &BarcodeWidget::showAbout);
    connect(debugMqttAction, &QAction::triggered, this, &BarcodeWidget::showMqttDebugMonitor);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15); // 调整控件之间的间距
    mainLayout->setContentsMargins(30, 20, 30, 20);

    mainLayout->setMenuBar(menuBar);

    auto* fileLayout = new QHBoxLayout();
    filePathEdit     = new QLineEdit(this);
    filePathEdit->setPlaceholderText("选择一个文件或图片");
    filePathEdit->setFont(QFont("Arial", 14));
    filePathEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #ccc; border-radius: 5px; padding: 5px; background-color: #f9f9f9; }");

    QPushButton* browseButton = new QPushButton("浏览", this);
    browseButton->setFixedWidth(100);
    browseButton->setFont(QFont("Arial", 16));
    browseButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border-radius: 5px; padding: 10px; }"
        "QPushButton:disabled { background-color: #ddd; }");
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);
    mainLayout->addLayout(fileLayout);

    // 生成与保存按钮
    auto* buttonLayout = new QHBoxLayout();
    generateButton     = new QPushButton("生成", this);
    decodeToChemFile   = new QPushButton("解码");
    saveButton         = new QPushButton("保存", this);
    generateButton->setFixedHeight(40);
    saveButton->setFixedHeight(40);
    decodeToChemFile->setFixedHeight(40);
    generateButton->setFont(QFont("Consolas", 16));
    decodeToChemFile->setFont(QFont("Consolas", 16));
    saveButton->setFont(QFont("Consolas", 16));

    generateButton->setToolTip("请选择任意文件来生成条码");
    decodeToChemFile->setToolTip("可以解码PNG图片中的条码");

    generateButton->setEnabled(false);
    decodeToChemFile->setEnabled(false);
    saveButton->setEnabled(false);

    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(decodeToChemFile);
    buttonLayout->addWidget(saveButton);
    mainLayout->addLayout(buttonLayout);


    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true); // 显示百分比文字
    progressBar->setVisible(false);    // 默认隐藏，只有批量处理时才显示
    // 可选：设置一点样式让它更好看
    progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #ccc; border-radius: 5px; text-align: center; height: 20px; }"
        "QProgressBar::chunk { background-color: #4CAF50; width: 1px; }");
    mainLayout->addWidget(progressBar);


    // 图片展示区域
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(320);
    scrollArea->setStyleSheet("QScrollArea { background-color: #f0f0f0; border: 1px solid #ccc; }");
    mainLayout->addWidget(scrollArea);

    base64CheckBox = new QCheckBox("Base64", this);
    base64CheckBox->setFont(QFont("Arial", 14));
    base64CheckBox->setChecked(true);
    buttonLayout->addWidget(base64CheckBox);

    directTextCheckBox = new QCheckBox("直接文本", this);
    directTextCheckBox->setFont(QFont("Arial", 14));
    directTextCheckBox->setToolTip("勾选后，输入框内的文字将直接作为条码内容，而不是文件路径");
    buttonLayout->addWidget(directTextCheckBox);

    auto* comboBoxLayout      = new QHBoxLayout();

    QComboBox* formatComboBox = new QComboBox(this);
    formatComboBox->setFont(QFont("Consolas", 13));

    for (const auto& item : qAsConst(barcodeFormats)) {
        formatComboBox->addItem(item);
    }

    QLabel* formatLabel = new QLabel("选择条码类型:", this);
    formatLabel->setFont(QFont("Consolas", 14));
    comboBoxLayout->addWidget(formatLabel);
    comboBoxLayout->addWidget(formatComboBox);
    mainLayout->addLayout(comboBoxLayout);

    auto* sizeLayout   = new QHBoxLayout();

    QLabel* widthLabel = new QLabel("宽度:", this);
    widthInput         = new QLineEdit(this);
    widthInput->setText("300"); // 默认宽度
    widthInput->setFont(QFont("Consolas", 13));
    widthInput->setStyleSheet(
        "QLineEdit { border: 1px solid #ccc; border-radius: 5px; padding: 5px; background-color: #f9f9f9; }");

    QLabel* heightLabel = new QLabel("高度:", this);
    heightInput         = new QLineEdit(this);
    heightInput->setText("300"); // 默认高度
    heightInput->setFont(QFont("Consolas", 13));
    heightInput->setStyleSheet(
        "QLineEdit { border: 1px solid #ccc; border-radius: 5px; padding: 5px; background-color: #f9f9f9; }");

    sizeLayout->addWidget(widthLabel);
    sizeLayout->addWidget(widthInput);
    sizeLayout->addWidget(heightLabel);
    sizeLayout->addWidget(heightInput);

    mainLayout->addLayout(sizeLayout);

    fileDialog = new QFileDialog(this, "Select File", "", "Supported Files (*.rfa *.txt *.png);;All Files (*)");
    fileDialog->setModal(false);

    MqttConfig config = MqttSubscriber::loadMqttConfig("./setting/config.json");

    subscriber_       = std::make_unique<MqttSubscriber>(
        config.host, config.port, config.client_id, [this](const std::string& topic, const std::string& payload) {
            emit mqttMessageReceived(QString::fromStdString(topic), QByteArray::fromStdString(payload));
        });
    subscriber_->subscribe("test/topic");

    messageWidget = std::make_unique<MQTTMessageWidget>();

    connect(browseButton, &QPushButton::clicked, this, &BarcodeWidget::onBrowseFile);
    connect(generateButton, &QPushButton::clicked, this, &BarcodeWidget::onGenerateClicked);
    connect(decodeToChemFile, &QPushButton::clicked, this, &BarcodeWidget::onDecodeToChemFileClicked);
    connect(saveButton, &QPushButton::clicked, this, &BarcodeWidget::onSaveClicked);
    connect(filePathEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        lastResults.clear();
        lastSelectedFiles = text.split(QDir::listSeparator());
        if(lastSelectedFiles.size() == 1) {
            if(lastSelectedFiles.front().isEmpty()) {
                //不知道为什么空字符串会产生一个元素。。
                lastSelectedFiles.clear();
            }
        }
        renderResults();
        updateButtonStates();
    });

    connect(formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, formatComboBox](int index) {
        // 设置当前选择的条码格式
        currentBarcodeFormat = stringToBarcodeFormat(barcodeFormats[index]);
    });
    connect(this, &BarcodeWidget::mqttMessageReceived, this, [this](const QString& topic, const QByteArray& payload) {
        //QMessageBox::information(this, "订阅消息", QString("主题: %1\n内容: %2").arg(topic, payload));
        messageWidget->addMessage(topic,payload);
    });

    connect(directTextCheckBox, &QCheckBox::stateChanged, this, [this, browseButton](int state) {
        filePathEdit->clear();
        lastSelectedFiles.clear();
        lastResults.clear();

        if(state) {
            filePathEdit->setPlaceholderText("输入要转换的文字");
            browseButton->setEnabled(false);
        }else {
            filePathEdit->setPlaceholderText("选择一个文件或图片");
            browseButton->setEnabled(true);
            decodeToChemFile->setEnabled(false);
        }

        renderResults();
        updateButtonStates();
    });

    connect(fileDialog, &QFileDialog::filesSelected, this, [this](const QStringList& filenames) {
        filePathEdit->clear();
        filePathEdit->setText(filenames.join(QDir::listSeparator()));
        lastSelectedFiles = filenames;
        lastResults.clear();
        renderResults();
        updateButtonStates();
    });

    renderResults();
}


void BarcodeWidget::updateButtonStates() const {
    saveButton->setEnabled(false);

    if(lastSelectedFiles.isEmpty()) {
        generateButton->setEnabled(false);
        decodeToChemFile->setEnabled(false);
    }else {
        if(lastSelectedFiles.size() == 1) {
            auto& file = lastSelectedFiles.front();

            bool isImage = fileExtensionRegex_image.match(file).hasMatch();
            generateButton->setEnabled(!isImage);
            decodeToChemFile->setEnabled(isImage);
        }else {
            generateButton->setEnabled(true);
            decodeToChemFile->setEnabled(true);
        }

    }
}

void BarcodeWidget::onBrowseFile() const {
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setOption(QFileDialog::ShowDirsOnly, false);
    fileDialog->setWindowTitle("选择需要转换的文件或图片");
    fileDialog->open();
}

void BarcodeWidget::onGenerateClicked() {
    const auto reqWidth  = widthInput->text().toInt();
    const auto reqHeight = heightInput->text().toInt();
    const auto useBase64 = base64CheckBox->isChecked();
    const auto format    = currentBarcodeFormat;

    if (directTextCheckBox->isChecked()) {
        QString rawText = filePathEdit->text();
        if (rawText.isEmpty()) return;

        // 构造一个包含单个元素的列表，以便复用 QtConcurrent::mapped
        // 这样可以不用重写 onBatchFinish 的逻辑
        QStringList inputs;
        inputs.append(rawText);

        // 准备 UI
        progressBar->setVisible(true);
        progressBar->setRange(0, 1);
        progressBar->setValue(0);
        generateButton->setEnabled(false);
        saveButton->setEnabled(false);
        this->setCursor(Qt::WaitCursor);

        // 定义处理文本的 Worker
        struct TextWorker {
            using result_type = convert::result_data_entry;

            bool useBase64;
            convert::QRcode_create_config config;

            convert::result_data_entry operator()(const QString& textInput) const {
                convert::result_data_entry res;
                // 对于直接文本模式，source_file_name 可以设为空，或者设为一个标识字符串
                // 这样在保存文件时，会默认生成 "qrcode.png" 之类的名字
                res.source_file_name = "raw_text_input";

                try {
                    std::string content;
                    if (useBase64) {
                        // 如果勾选了 Base64，先将输入文本转为 UTF-8 字节流，再 Base64 编码
                        QByteArray data = textInput.toUtf8();
                        content = SimpleBase64::encode(reinterpret_cast<const std::uint8_t*>(data.constData()), data.size());
                    } else {
                        content = textInput.toStdString();
                    }

                    auto img = convert::byte_to_QRCode_qimage(
                        content, config);

                    if (!img.isNull()) {
                        res.data = img;
                    } else {
                        res.data = std::string("生成图片失败");
                    }
                } catch (const std::exception& e) {
                    res.data.emplace<std::string>(e.what());
                }
                return res;
            }
        };

        auto* watcher = new QFutureWatcher<convert::result_data_entry>(this);
        connect(watcher, &QFutureWatcher<convert::result_data_entry>::finished,
            [this, watcher] { onBatchFinish(*watcher); });

        // 启动异步任务
        watcher->setFuture(QtConcurrent::mapped(inputs, TextWorker{useBase64, {reqWidth, reqHeight, format}}));

        return; // 结束函数，不再执行下方的文件处理逻辑
    }

    QStringList filePaths;
    filePaths.reserve(static_cast<int>(lastResults.size()));

    //因为先前的逻辑是不是图片就算文本，所以先这样吧
    std::ranges::copy(lastSelectedFiles | std::views::filter([](const QString& file) {
        return !fileExtensionRegex_image.match(file).hasMatch();
    }), std::back_inserter(filePaths));
    if (filePaths.empty()) {
        QMessageBox::warning(this, "警告", "无可处理文件");
        return;
    }
    // 2. UI 状态准备
    progressBar->setVisible(true);
    progressBar->setRange(0, filePaths.size()); // 设置进度条范围
    progressBar->setValue(0);
    generateButton->setEnabled(false);
    decodeToChemFile->setEnabled(false);
    saveButton->setEnabled(false);
    this->setCursor(Qt::WaitCursor);

    struct worker {
        using result_type = convert::result_data_entry;
        int reqWidth;
        int reqHeight;
        bool useBase64;
        ZXing::BarcodeFormat format;

        convert::result_data_entry operator()(const QString& filePath) const {
            try {
                QFile file(filePath);

                convert::result_data_entry res;
                if (!file.open(QIODevice::ReadOnly)) {
                    res.data = std::string("无法打开文件: ") + filePath.toStdString();
                    res.source_file_name = std::move(filePath);
                    return res;
                } else {
                    res.source_file_name = std::move(filePath);
                }

                const QByteArray data = file.readAll();
                file.close();

                // 是否base64处理通过判断base64CheckBox
                std::string text;
                if (useBase64) {
                    text = SimpleBase64::encode(reinterpret_cast<const std::uint8_t*>(data.constData()), data.size());
                } else {
                    text = data.toStdString();
                }

                auto img = convert::byte_to_QRCode_qimage(
                    text, {.target_width = reqWidth, .target_height = reqHeight, .format = format, .margin = 1});

                if (!img.isNull()) {
                    res.data = img;
                } else {
                    res.data = std::string("生成图片失败");
                }

                return res;
            } catch (const std::exception& e) {
                convert::result_data_entry res;
                res.source_file_name = std::move(filePath);
                res.data.emplace<std::string>(e.what());
                return res;
            }
        }
    };

    auto* watcher = new QFutureWatcher<convert::result_data_entry>(this);

    connect(watcher, &QFutureWatcher<convert::result_data_entry>::progressValueChanged, progressBar, &QProgressBar::setValue);

    connect(watcher, &QFutureWatcher<convert::result_data_entry>::finished, 
        [this, watcher] {
            onBatchFinish(*watcher);
        }
    );

    watcher->setFuture(QtConcurrent::mapped(filePaths, worker{reqWidth, reqHeight, useBase64, format}));
}

void BarcodeWidget::onDecodeToChemFileClicked() {

    const QStringList filePaths = lastSelectedFiles.filter(fileExtensionRegex_image);
    if (filePaths.empty()) {
        QMessageBox::warning(this, "警告", "无可处理文件");
        return;
    }

    // 2. UI 状态准备
    progressBar->setVisible(true);
    progressBar->setRange(0, filePaths.size()); // 设置进度条范围
    progressBar->setValue(0);
    generateButton->setEnabled(false);
    decodeToChemFile->setEnabled(false);
    saveButton->setEnabled(false);
    this->setCursor(Qt::WaitCursor);

    struct worker {
        using result_type = convert::result_data_entry;

        bool useBase64;
        convert::result_data_entry operator()(QString path) const {
            try {
                const auto bytes = path.toLocal8Bit().toStdString();
                switch (auto rst = convert::QRcode_to_byte(bytes); rst.err) {
                case convert::result_i2t::invalid_qrcode:
                    spdlog::error("loadImageFromFile 无法加载图片文件: {}", path.toStdString());
                    return {std::move(path), QString{"无法加载图片文件: %1"}.arg(path).toStdString()};
                case convert::result_i2t::empty_img:
                    return {std::move(path), std::string{"无法识别条码或条码格式不正确"}};
                default:
                    std::vector<std::uint8_t> decodedData;
                    if (useBase64) {
                        decodedData = SimpleBase64::decode(rst.text);
                    } else {
                        decodedData = std::vector<std::uint8_t>(rst.text.begin(), rst.text.end());
                    }
                    return {std::move(path),
                        QByteArray(reinterpret_cast<const char*>(decodedData.data()), decodedData.size())};
                }
            } catch (const std::exception& e) {
                return {std::move(path), QString("解码失败:\n%1").arg(e.what()).toStdString()};
            }
        }
    };

    auto* watcher = new QFutureWatcher<convert::result_data_entry>(this);

    connect(watcher, &QFutureWatcher<convert::result_data_entry>::progressValueChanged,
        progressBar, &QProgressBar::setValue);

    connect(watcher, &QFutureWatcher<convert::result_data_entry>::finished,
        [this, watcher] { onBatchFinish(*watcher); });

    watcher->setFuture(QtConcurrent::mapped(filePaths, worker{base64CheckBox->isChecked()}));
}

void BarcodeWidget::onSaveClicked() {
    if (lastResults.empty()) {
        QMessageBox::warning(this, "警告", "没有可保存的内容。");
        return;
    }

    struct SaveTask {
        convert::result_data_entry entry;
        QString                    dest;
    };

    struct SaveResult {
        enum errcode {
            success,
            invalid_data,
            failed,
        };

        errcode err;
        QString path;
    };

    QList<SaveTask> tasks;

    if (lastResults.size() == 1) {
        const auto& entry = lastResults.front();

        const QString defName   = entry.get_default_target_name();
        auto fileName  = std::visit<QString>(
            overload_def_noop{std::in_place_type<QString>,
                [&](const QImage&) {
                    return QFileDialog::getSaveFileName(this, "保存图片", defName, "PNG Images (*.png)");
                },
                [&](const QByteArray&) {
                    return QFileDialog::getSaveFileName(
                        this, "保存文件", defName, "Binary Files (*.rfa);;Text Files (*.txt)");
                }},
            entry.data);

        if (fileName.isEmpty())
            return;
        tasks.append({entry, std::move(fileName)});
    } else {
        const QString dir = QFileDialog::getExistingDirectory(this, "请选择保存文件夹",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (dir.isEmpty())
            return;

        const QDir outputDir(dir);
        for (const auto& entry : lastResults) {
            if (!entry) {
                continue;
            }

            const QString fileName = outputDir.filePath(entry.get_default_target_name());
            tasks.append({entry, std::move(fileName)});
        }
    }

    if (tasks.isEmpty())
        return;

    progressBar->setVisible(true);
    progressBar->setRange(0, tasks.size());
    progressBar->setValue(0);
    saveButton->setEnabled(false);
    generateButton->setEnabled(false);
    decodeToChemFile->setEnabled(false);
    this->setCursor(Qt::WaitCursor);

    struct worker {
        using result_type = SaveResult;
        SaveResult operator()(const SaveTask& task) const noexcept try {
            return std::visit<SaveResult>(
                overload_def_noop{std::in_place_type<SaveResult>,
                    [&](const QImage& img) -> SaveResult {
                        if (img.isNull())
                            return {SaveResult::invalid_data, task.dest};
                        if (img.save(task.dest)) {
                            return {SaveResult::success, task.dest};
                        } else {
                            return {SaveResult::failed, task.dest};
                        }
                    },
                    [&](const QByteArray& data) -> SaveResult {
                        if (data.isEmpty())
                            return {SaveResult::invalid_data, task.dest};

                        QFile f(task.dest);
                        if (f.open(QIODevice::WriteOnly)) {
                            f.write(data);
                            f.close();
                            return {SaveResult::success, task.dest};
                        }
                        return {SaveResult::failed, task.dest};
                    },
                    [&](const auto&) noexcept { return SaveResult{SaveResult::failed, task.dest}; }},
                task.entry.data);
        } catch (...) {
            return {SaveResult::failed};
        }
    };

    auto* watcher = new QFutureWatcher<SaveResult>(this);

    connect(watcher, &QFutureWatcher<SaveResult>::progressValueChanged, progressBar, &QProgressBar::setValue);

    connect(watcher, &QFutureWatcher<SaveResult>::finished, [this, watcher]() {
        this->setCursor(Qt::ArrowCursor);
        progressBar->setVisible(false);

        // 恢复按钮状态
        updateButtonStates();
        saveButton->setEnabled(true);
        // 保存按钮总是可以再次点击

        auto list = watcher->future().results();

        int successCount = 0;
        QStringList failedInfos;
        QStringList successInfos;

        for (const auto& res : list) {
            QString fileName = QFileInfo(res.path).fileName();

            if (res.err == SaveResult::success) {
                successCount++;
                // 如果总数不多，记录成功的文件名用于展示
                if (list.size() <= 10) {
                    successInfos.append(fileName);
                }
            } else {
                QString reason;
                switch (res.err) {
                case SaveResult::invalid_data:
                    reason = "数据为空或无效";
                    break;
                case SaveResult::failed:
                    reason = "写入失败";
                    break;
                default:
                    reason = "未知错误";
                    break;
                }
                failedInfos.append(QString("• %1 (%2)").arg(fileName, reason));
            }
        }

        // 构建消息框内容
        QString msg = QString("操作完成。\n总计处理: %1\n成功: %2\n失败: %3")
                          .arg(list.size())
                          .arg(successCount)
                          .arg(failedInfos.size());

        if (!failedInfos.isEmpty()) {
            msg                                  += "\n\n[保存失败的文件]:\n";
            // 限制显示的错误行数，防止弹窗过高
            static constexpr int maxErrorsToShow  = 10;
            for (int i = 0; i < std::min(failedInfos.size(), maxErrorsToShow); ++i) {
                msg += failedInfos[i] + "\n";
            }
            if (failedInfos.size() > maxErrorsToShow) {
                msg += QString("...以及其他 %1 个文件").arg(failedInfos.size() - maxErrorsToShow);
            }
            QMessageBox::warning(this, "保存结果 - 包含错误", msg);
        } else {
            // 全部成功的情况
            if (!successInfos.isEmpty() && list.size() > 1) {
                msg += "\n\n[文件列表]:\n" + successInfos.join("\n");
            }
            QMessageBox::information(this, "保存成功", msg);
        }

        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::mapped(tasks, worker{}));
}

void BarcodeWidget::showAbout() const {
    const QString tag = version::git_tag.data();
    const QString hash = version::git_hash.data();
    const QString branch = version::git_branch.data();
    const QString commitTime = version::git_commit_time.data();
    const QString buildTime = version::build_time.data();

    AboutDialog* aboutDialog = new AboutDialog();
    aboutDialog->setVersionInfo(tag, hash, branch, commitTime, buildTime);
    aboutDialog->exec();
    aboutDialog->deleteLater();
}

void BarcodeWidget::showMqttDebugMonitor() const {
    messageWidget->show();
}

QImage BarcodeWidget::MatToQImage(const cv::Mat& mat) const {
    if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8).copy();
    else if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }
    return {};
}

void BarcodeWidget::renderResults() const {
    QWidget* container = new QWidget();
    // 容器背景设为透明或跟随 ScrollArea
    container->setStyleSheet("background-color: transparent;");

    if (lastResults.empty() && directTextCheckBox->isChecked()) {
        QVBoxLayout* vLayout = new QVBoxLayout(container);
        QLabel* infoLabel = new QLabel("当前模式：直接文本生成\n请输入内容并点击生成");
        infoLabel->setAlignment(Qt::AlignCenter);
        infoLabel->setStyleSheet("font-size: 18px; color: #aaa;");
        vLayout->addWidget(infoLabel);
        scrollArea->setWidget(container);
        return;
    }

    constexpr int maxColumns = 4; // 每行最多4个 (仅用于多结果)

    if (lastResults.empty()) {
        if (!lastSelectedFiles.empty()) {
            QVBoxLayout* listLayout = new QVBoxLayout(container);
            listLayout->setContentsMargins(10, 10, 10, 10);
            listLayout->setAlignment(Qt::AlignTop);

            QLabel* headerLabel = new QLabel(QString("已选择 %1 个文件，准备处理:").arg(lastSelectedFiles.size()));
            headerLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #555; margin-bottom: 5px;");
            listLayout->addWidget(headerLabel);

            for (const QString& filePath : lastSelectedFiles) {
                QFileInfo fi(filePath);
                QString fileName = fi.fileName();

                // 判断文件类型以决定图标和操作提示
                bool isText  = fileExtensionRegex_text.match(fileName).hasMatch();
                bool isImage = fileExtensionRegex_image.match(fileName).hasMatch();

                // 创建单行容器 Widget
                QWidget* rowWidget = new QWidget();
                rowWidget->setStyleSheet(
                    "QWidget {"
                    "   border: 1px solid #ccc;"
                    "   background-color: white;"
                    "   border-radius: 4px;"
                    "}"
                    "QWidget:hover {"
                    "   background-color: #f0f9ff;"
                    "   border-color: #40a9ff;"
                    "}"
                );

                QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
                rowLayout->setContentsMargins(10, 8, 10, 8);
                rowLayout->setSpacing(12);

                // 1. 图标 Label
                QLabel* iconLabel = new QLabel();
                iconLabel->setFixedSize(24, 24);
                // 清除父级样式表对 QLabel 边框的影响
                iconLabel->setStyleSheet("border: none; background: transparent;");

                // 2. 使用 QPainter 绘制动态图标 (避免依赖外部图片资源)
                QPixmap iconPix(24, 24);
                iconPix.fill(Qt::transparent);
                {
                    QPainter p(&iconPix);
                    p.setRenderHint(QPainter::Antialiasing);

                    drawIcon(p, isImage, isText);
                }
                iconLabel->setPixmap(iconPix);

                // 3. 文件名
                QLabel* nameLabel = new QLabel(fileName);
                nameLabel->setStyleSheet("border: none; background: transparent; font-family: Consolas; font-size: 14px; color: #333;");
                nameLabel->setToolTip(filePath); // 鼠标悬停显示完整路径

                // 4. 类型/操作提示标签
                QLabel* typeLabel = new QLabel();
                typeLabel->setStyleSheet("border: none; background: transparent; font-size: 12px; font-weight: bold;");

                if (isImage) {
                    typeLabel->setText("[待解码]");
                    typeLabel->setStyleSheet(typeLabel->styleSheet() + "color: #67C23A;"); // 橙色提示解码
                } else if (isText) {
                    typeLabel->setText("[待生成]");
                    typeLabel->setStyleSheet(typeLabel->styleSheet() + "color: #67C23A;"); // 绿色提示生成
                } else {
                    typeLabel->setText("[不确定类型，默认待生成]");
                    typeLabel->setStyleSheet(typeLabel->styleSheet() + "color: #E6A23C;");
                }

                // 布局添加
                rowLayout->addWidget(iconLabel);
                rowLayout->addWidget(nameLabel, 1); // 这里的 1 代表拉伸因子，让文件名占据主要空间
                rowLayout->addWidget(typeLabel);

                listLayout->addWidget(rowWidget);
            }
            // 底部弹簧，确保列表靠上
            listLayout->addStretch();

        } else {
            // 无文件选中且无结果
            QVBoxLayout* vLayout = new QVBoxLayout(container);
            QLabel* emptyLabel = new QLabel("请选择文件\n或者键入内容");
            emptyLabel->setAlignment(Qt::AlignCenter);
            emptyLabel->setStyleSheet("font-size: 18px; color: #aaa;");
            vLayout->addWidget(emptyLabel);
        }
    } else if (lastResults.size() == 1) {
        // --- 单个结果展示逻辑 (保持原样) ---
       const auto& entry = lastResults.front();
        QVBoxLayout* singleLayout = new QVBoxLayout(container);
        singleLayout->setContentsMargins(10, 10, 10, 10);
        singleLayout->setSpacing(5);

        QWidget* contentWidget = nullptr;

        std::visit(overload_def_noop{
                std::in_place_type<void>,
                [&](const QImage& img) {
                    QLabel* imgLabel = new QLabel();
                    imgLabel->setPixmap(QPixmap::fromImage(img));
                    imgLabel->setAlignment(Qt::AlignCenter);
                    imgLabel->setStyleSheet("border: 1px solid #ddd; background: white;");
                    imgLabel->setToolTip(QString("Size: %1x%2").arg(img.width()).arg(img.height()));

                    // [修改] 将最小尺寸设置为图片尺寸，确保大图能撑开 ScrollArea 出现滚动条
                    imgLabel->setMinimumSize(img.size());

                    contentWidget = imgLabel;
                },
                [&](const QByteArray& data) {
                    QLabel* textLabel = new QLabel();
                    // 显示完整解码内容
                    QString textDisplay = QString::fromUtf8(data);

                    textLabel->setText(textDisplay);
                    textLabel->setWordWrap(true);

                    textLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

                    // 确保不使用 QTextEdit，这里使用 QLabel
                    textLabel->setStyleSheet(
                        "border: 1px solid #ddd; background: white; padding: 15px; font-family: Consolas; font-size: 14pt;");
                    textLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // 允许内容撑开
                    contentWidget = textLabel;
                },
                [&](const std::string& str) {
                    QLabel* errLabel = new QLabel(QString::fromStdString(str));
                    errLabel->setStyleSheet(
                        "color: red; border: 1px solid red; background: #fff0f0; padding: 15px; font-size: 14pt;");
                    errLabel->setWordWrap(true);

                    errLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                    errLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                    contentWidget = errLabel;
                }
            },
            entry.data);

        if(contentWidget) {
            singleLayout->addWidget(contentWidget, 1); // 权重设为 1 占据空间
        }
    } else {
        // --- 多个结果网格展示逻辑 (保持原样) ---
        QGridLayout* gridLayout = new QGridLayout(container);
        gridLayout->setHorizontalSpacing(20);
        gridLayout->setVerticalSpacing(40);
        gridLayout->setContentsMargins(20, 20, 20, 20);

        int count = 0;

        for (const auto& entry : lastResults) {
            int row = count / maxColumns;
            int col = count % maxColumns;

            QWidget* cellWidget = new QWidget();
            QVBoxLayout* cellLayout = new QVBoxLayout(cellWidget);
            cellLayout->setContentsMargins(0, 0, 0, 0);
            cellLayout->setSpacing(5);

            QWidget* contentWidget = nullptr;

            std::visit(overload_def_noop{ std::in_place_type<void>,
                // 图片类型，显示缩略图
               [&](const QImage& img) {
                   QLabel* imgLabel = new QLabel();
                   // 使用缩略图大小 200x200
                   imgLabel->setPixmap(QPixmap::fromImage(img).scaled(
                       200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                   imgLabel->setAlignment(Qt::AlignCenter);
                   imgLabel->setStyleSheet("border: 1px solid #ddd; background: white;");
                   imgLabel->setToolTip(QString("Size: %1x%2").arg(img.width()).arg(img.height()));
                   contentWidget = imgLabel;
               },
                // 文本类型，显示前200字符 (截断)
               [&](const QByteArray& data) {
                   QLabel* textLabel = new QLabel();
                   QString textDisplay = QString::fromUtf8(data);
                   if (textDisplay.length() > 256) {
                       textDisplay = textDisplay.left(256) + "...";
                   }
                   textLabel->setText(textDisplay);
                   textLabel->setWordWrap(true);

                   textLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

                   textLabel->setStyleSheet(
                       "border: 1px solid #ddd; background: white; padding: 5px; font-family: Consolas;");
                   textLabel->setFixedSize(200, 200);
                   contentWidget = textLabel;
               },
                // 错误信息，显示解码或生成的错误内容
               [&](const std::string& str) {
                   QLabel* errLabel = new QLabel(QString::fromStdString(str));
                   errLabel->setStyleSheet(
                       "font-size: 15pt; color: red; border: 1px solid red; background: #fff0f0; padding: 5px;");
                   errLabel->setWordWrap(true);

                   errLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

                   errLabel->setFixedSize(200, 200);
                   contentWidget = errLabel;
               } },
                entry.data);

            if (contentWidget) {
                // 仅在多结果模式下渲染文件名
                QString fileNameStr = QFileInfo(entry.source_file_name).fileName();
                if (fileNameStr.isEmpty())
                    fileNameStr = "Unknown";

                QLabel* nameLabel = new QLabel(fileNameStr);
                nameLabel->setAlignment(Qt::AlignCenter);
                nameLabel->setStyleSheet("font-size: 10pt; color: #333; font-weight: bold;");
                nameLabel->setFixedWidth(200);
                nameLabel->setToolTip(entry.source_file_name);


                QFontMetrics metrics(nameLabel->font());
                QString      elidedText = metrics.elidedText(fileNameStr, Qt::ElideMiddle, 200);
                nameLabel->setText(elidedText);

                cellLayout->addWidget(nameLabel);

                cellLayout->addWidget(contentWidget);

                gridLayout->addWidget(cellWidget, row, col, Qt::AlignTop);
            }
            count++;
        }
    }

    scrollArea->setWidget(container);
}

void BarcodeWidget::onBatchFinish(QFutureWatcher<convert::result_data_entry>& watcher) {
    setCursor(Qt::ArrowCursor);
    if(lastSelectedFiles.size() == 1) {
        auto& file = lastSelectedFiles.front();
        bool isImage = fileExtensionRegex_image.match(file).hasMatch();
        generateButton->setEnabled(!isImage);
        decodeToChemFile->setEnabled(isImage);
    }else if(!lastSelectedFiles.isEmpty()){
        generateButton->setEnabled(true);
        decodeToChemFile->setEnabled(true);
    }

    progressBar->setVisible(false);

    QList<convert::result_data_entry> results = watcher.future().results();

    lastResults.clear();
    lastResults.reserve(results.size());
    for (auto& item : results) {
        lastResults.push_back(std::move(item));
    }

    if (!lastResults.empty()) {
        saveButton->setEnabled(true);
        renderResults(); // 批量渲染结果
    }

    watcher.deleteLater();
}

QString BarcodeWidget::barcodeFormatToString(ZXing::BarcodeFormat format) {
    static const QMap<ZXing::BarcodeFormat, QString> map = {
        {ZXing::BarcodeFormat::None,            "None"           },
        {ZXing::BarcodeFormat::Aztec,           "Aztec"          },
        {ZXing::BarcodeFormat::Codabar,         "Codabar"        },
        {ZXing::BarcodeFormat::Code39,          "Code39"         },
        {ZXing::BarcodeFormat::Code93,          "Code93"         },
        {ZXing::BarcodeFormat::Code128,         "Code128"        },
        {ZXing::BarcodeFormat::DataBar,         "DataBar"        },
        {ZXing::BarcodeFormat::DataBarExpanded, "DataBarExpanded"},
        {ZXing::BarcodeFormat::DataMatrix,      "DataMatrix"     },
        {ZXing::BarcodeFormat::EAN8,            "EAN8"           },
        {ZXing::BarcodeFormat::EAN13,           "EAN13"          },
        {ZXing::BarcodeFormat::ITF,             "ITF"            },
        {ZXing::BarcodeFormat::MaxiCode,        "MaxiCode"       },
        {ZXing::BarcodeFormat::PDF417,          "PDF417"         },
        {ZXing::BarcodeFormat::QRCode,          "QRCode"         },
        {ZXing::BarcodeFormat::UPCA,            "UPCA"           },
        {ZXing::BarcodeFormat::UPCE,            "UPCE"           },
        {ZXing::BarcodeFormat::MicroQRCode,     "MicroQRCode"    },
        {ZXing::BarcodeFormat::RMQRCode,        "RMQRCode"       },
        {ZXing::BarcodeFormat::DXFilmEdge,      "DXFilmEdge"     },
        {ZXing::BarcodeFormat::DataBarLimited,  "DataBarLimited" }
    };

    return map.value(format, "Unknown");
}


ZXing::BarcodeFormat BarcodeWidget::stringToBarcodeFormat(const QString& formatStr) {
    static const QMap<QString, ZXing::BarcodeFormat> map = {
        {"Aztec",           ZXing::BarcodeFormat::Aztec          },
        {"Codabar",         ZXing::BarcodeFormat::Codabar        },
        {"Code39",          ZXing::BarcodeFormat::Code39         },
        {"Code93",          ZXing::BarcodeFormat::Code93         },
        {"Code128",         ZXing::BarcodeFormat::Code128        },
        {"DataBar",         ZXing::BarcodeFormat::DataBar        },
        {"DataBarExpanded", ZXing::BarcodeFormat::DataBarExpanded},
        {"DataMatrix",      ZXing::BarcodeFormat::DataMatrix     },
        {"EAN8",            ZXing::BarcodeFormat::EAN8           },
        {"EAN13",           ZXing::BarcodeFormat::EAN13          },
        {"ITF",             ZXing::BarcodeFormat::ITF            },
        {"MaxiCode",        ZXing::BarcodeFormat::MaxiCode       },
        {"PDF417",          ZXing::BarcodeFormat::PDF417         },
        {"QRCode",          ZXing::BarcodeFormat::QRCode         },
        {"UPCA",            ZXing::BarcodeFormat::UPCA           },
        {"UPCE",            ZXing::BarcodeFormat::UPCE           },
        {"MicroQRCode",     ZXing::BarcodeFormat::MicroQRCode    },
        {"RMQRCode",        ZXing::BarcodeFormat::RMQRCode       },
        {"DXFilmEdge",      ZXing::BarcodeFormat::DXFilmEdge     },
        {"DataBarLimited",  ZXing::BarcodeFormat::DataBarLimited }
    };

    QString key = formatStr.trimmed();
    key[0]      = key[0].toUpper(); // 确保首字母大写以匹配上面的key
    auto it     = map.find(key);
    if (it != map.end())
        return it.value();

    return ZXing::BarcodeFormat::None; // 未匹配时返回None
}
