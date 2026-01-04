#include "BarcodeWidget.h"
#include "LanguageManager.h"
#include "about_dialog.h"
#include "components/UiConfig.h"
#include "components/message_dialog.h"
#include "convert.h"
#include "version_info/version.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QtConcurrent>
#include <SimpleBase64.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/TextUtfEncoding.h>
#include <magic_enum/magic_enum.hpp>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <spdlog/spdlog.h>

template <typename Ret, typename... Fs>
requires(std::is_void_v<Ret> || std::is_default_constructible_v<Ret>)
struct overload_def_noop : private Fs... {
    template <typename V, typename... Args>
    constexpr explicit(false) overload_def_noop(std::in_place_type_t<V>, Args &&...fs)
        : Fs{std::forward<Args>(fs)}... {}

    using Fs::operator()...;

    template <typename... T>
    requires(!std::invocable<Fs, T...> && ...)
    Ret operator()(T &&...) const noexcept {
        if constexpr (std::is_void_v<Ret>) {
            return;
        } else {
            return Ret{};
        }
    }
};

template <typename V, typename... Fs>
overload_def_noop(std::in_place_type_t<V>, Fs &&...) -> overload_def_noop<V, std::decay_t<Fs>...>;

void drawIcon(QPainter &p, bool isImage, bool isText) {
    if (isImage) {
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

static QRegularExpression fileExtensionRegex_text(R"(^.*\.(?:txt|json|rfa)$)",
                                                  QRegularExpression::CaseInsensitiveOption);

static QRegularExpression fileExtensionRegex_image(R"(^.*\.(?:png|jpg|jpeg|bmp|gif|tiff|webp)$)",
                                                   QRegularExpression::CaseInsensitiveOption);

BarcodeWidget::BarcodeWidget(QWidget *parent)
    : QWidget(parent) {
    setWindowTitle("Lab2QRCode");
    setMinimumSize(500, 600);

    menuBar = new QMenuBar();
    helpMenu = menuBar->addMenu(tr("帮助"));
    toolsMenu = menuBar->addMenu(tr("工具"));
    settingMenu = menuBar->addMenu(tr("设置"));
    languageSubMenu = settingMenu->addMenu(tr("语言"));

    menuBar->setFont(Ui::getAppFont(12));

    aboutAction = new QAction(tr("关于软件"), this);
    debugMqttAction = new QAction(tr("MQTT实时消息监控窗口"), this);
    openCameraScanAction = new QAction(tr("打开摄像头扫码"), this);
    // base64勾选，默认勾选
    base64CheckAcion = new QAction(tr("Base64"), this);
    base64CheckAcion->setCheckable(true);
    base64CheckAcion->setChecked(true); // 默认勾选

    directTextAction = new QAction(tr("文本输入"), this);
    directTextAction->setCheckable(true);
    directTextAction->setChecked(false); // 默认不勾选

    helpMenu->addAction(aboutAction);
    toolsMenu->addAction(debugMqttAction);
    toolsMenu->addAction(openCameraScanAction);
    settingMenu->addAction(base64CheckAcion);
    settingMenu->addAction(directTextAction);

    // 连接菜单项的点击信号
    connect(aboutAction, &QAction::triggered, this, &BarcodeWidget::showAbout);
    connect(debugMqttAction, &QAction::triggered, this, &BarcodeWidget::showMqttDebugMonitor);
    connect(openCameraScanAction, &QAction::triggered, this, [this] {
        spdlog::info("BarcodeWidget openCameraScanAction triggered");
        preview.startCamera();
        preview.show();
    });

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15); // 调整控件之间的间距
    mainLayout->setContentsMargins(30, 20, 30, 20);

    mainLayout->setMenuBar(menuBar);

    auto *fileLayout = new QHBoxLayout();
    filePathEdit = new QLineEdit(this);
    filePathEdit->setObjectName("filePathEdit");
    filePathEdit->setPlaceholderText(tr("选择一个文件或图片"));
    filePathEdit->setFont(Ui::getAppFont(14));

    browseButton = new QPushButton(tr("浏览"), this);
    browseButton->setObjectName("browseButton");
    browseButton->setFixedWidth(100);
    browseButton->setFont(Ui::getAppFont(16));
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);
    mainLayout->addLayout(fileLayout);

    // 生成与保存按钮
    auto *buttonLayout = new QHBoxLayout();
    generateButton = new QPushButton(tr("生成"), this);
    decodeToChemFile = new QPushButton(tr("解码"));
    saveButton = new QPushButton(tr("保存"), this);
    generateButton->setFixedHeight(40);
    saveButton->setFixedHeight(40);
    decodeToChemFile->setFixedHeight(40);
    generateButton->setFont(Ui::getAppFont(16));
    decodeToChemFile->setFont(Ui::getAppFont(16));
    saveButton->setFont(Ui::getAppFont(16));

    generateButton->setToolTip(tr("请选择任意文件来生成条码"));
    decodeToChemFile->setToolTip(tr("可以解码PNG图片中的条码"));

    generateButton->setEnabled(false);
    decodeToChemFile->setEnabled(false);
    saveButton->setEnabled(false);

    emptyLabel = new QLabel(tr("请选择文件\n或者键入内容"));
    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(decodeToChemFile);
    buttonLayout->addWidget(saveButton);
    mainLayout->addLayout(buttonLayout);

    progressBar = new QProgressBar(this);
    progressBar->setObjectName("progressBar");
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true); // 显示百分比文字
    progressBar->setVisible(false);    // 默认隐藏，只有批量处理时才显示
    mainLayout->addWidget(progressBar);

    // 图片展示区域
    scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("scrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(320);
    mainLayout->addWidget(scrollArea);

    auto *comboBoxLayout = new QHBoxLayout();

    QComboBox *formatComboBox = new QComboBox(this);
    formatComboBox->setFont(Ui::getAppFont(13));

    for (const auto &item : qAsConst(barcodeFormats)) {
        formatComboBox->addItem(item);
    }
    formatComboBox->setCurrentText("QRCode");

    formatLabel = new QLabel(tr("选择条码类型:"), this);
    formatLabel->setFont(Ui::getAppFont(14));
    comboBoxLayout->addWidget(formatLabel);
    comboBoxLayout->addWidget(formatComboBox);
    mainLayout->addLayout(comboBoxLayout);

    auto *sizeLayout = new QHBoxLayout();

    widthLabel = new QLabel(tr("宽度:"), this);
    widthInput = new QLineEdit(this);
    widthInput->setObjectName("widthInput");
    widthInput->setText("300"); // 默认宽度
    widthInput->setFont(Ui::getAppFont(13));

    heightLabel = new QLabel(tr("高度:"), this);
    heightInput = new QLineEdit(this);
    heightInput->setObjectName("heightInput");
    heightInput->setText("300"); // 默认高度
    heightInput->setFont(Ui::getAppFont(13));

    sizeLayout->addWidget(widthLabel);
    sizeLayout->addWidget(widthInput);
    sizeLayout->addWidget(heightLabel);
    sizeLayout->addWidget(heightInput);

    mainLayout->addLayout(sizeLayout);

    fileDialog = new QFileDialog(this, "Select File", "", "Supported Files (*.rfa *.txt *.png);;All Files (*)");
    fileDialog->setModal(false);

    MqttConfig config = MqttSubscriber::loadMqttConfig("./setting/config.json");

    subscriber_ = std::make_unique<MqttSubscriber>(
        config.host, config.port, config.client_id, [this](const std::string &topic, const std::string &payload) {
            emit mqttMessageReceived(QString::fromStdString(topic), QByteArray::fromStdString(payload));
        });
    subscriber_->subscribe("test/topic");

    messageWidget = std::make_unique<MQTTMessageWidget>();

    connect(browseButton, &QPushButton::clicked, this, &BarcodeWidget::onBrowseFile);
    connect(generateButton, &QPushButton::clicked, this, &BarcodeWidget::onGenerateClicked);
    connect(decodeToChemFile, &QPushButton::clicked, this, &BarcodeWidget::onDecodeToChemFileClicked);
    connect(saveButton, &QPushButton::clicked, this, &BarcodeWidget::onSaveClicked);
    connect(filePathEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        lastResults.clear();
        lastSelectedFiles = text.split(QDir::listSeparator());
        if (lastSelectedFiles.size() == 1) {
            if (lastSelectedFiles.front().isEmpty()) {
                //不知道为什么空字符串会产生一个元素。。
                lastSelectedFiles.clear();
            }
        }
        renderResults();
        updateButtonStates();
    });

    connect(formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        // 设置当前选择的条码格式
        currentBarcodeFormat = stringToBarcodeFormat(barcodeFormats[index]);
    });
    connect(this, &BarcodeWidget::mqttMessageReceived, this, [this](const QString &topic, const QByteArray &payload) {
        // 记录收到的消息
        spdlog::info("MQTT Received: topic={}, payload={}", topic.toStdString(), payload.toStdString());

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            spdlog::warn(
                "JSON解析失败: {} , payload={}", parseError.errorString().toStdString(), payload.toStdString());
            messageWidget->addMessage(topic, payload); // 仍然显示原始消息
            return;
        }

        QJsonObject obj = doc.object();
        QString type = obj.value("type").toString();
        QString content = obj.value("content").toString();

        // 根据 type 弹窗
        if (type == "info") {
            MessageDialog::information(this, QString("主题: %1").arg(topic), content);
        } else if (type == "update") {
            QWidget *mask = new QWidget(this);
            mask->setStyleSheet("background-color: rgba(0, 0, 0,100);");
            mask->resize(this->size());
            mask->show();
            mask->raise();
            MessageDialog::updateDialog(this, "更新", content, {"忽略"});
            mask->deleteLater();
        }
        messageWidget->addMessage(topic, payload);
    });

    connect(directTextAction, &QAction::toggled, this, [this](bool checked) {
        filePathEdit->clear();
        lastSelectedFiles.clear();
        lastResults.clear();

        if (checked) {
            filePathEdit->setPlaceholderText(tr("输入要转换的文字"));
            browseButton->setEnabled(false);
        } else {
            filePathEdit->setPlaceholderText(tr("选择一个文件或图片"));
            browseButton->setEnabled(true);
            decodeToChemFile->setEnabled(false);
        }

        renderResults();
        updateButtonStates();
    });

    connect(fileDialog, &QFileDialog::filesSelected, this, [this](const QStringList &filenames) {
        filePathEdit->clear();
        filePathEdit->setText(filenames.join(QDir::listSeparator()));
        lastSelectedFiles = filenames;
        lastResults.clear();
        renderResults();
        updateButtonStates();
    });

    renderResults();

    // 这三行必须在构造函数最后面
    // 因为init会调用switchLanguage触发QEvent::LanguageChange事件
    // 从而BarCodeWidget::retranslate函数被调用
    // BarCodeWidget::retranslate函数调用前需确保BarCodeWidget的所有组件完成初始化
    // 否则会访问空指针导致程序崩溃
    LanguageManager &languageMgr = LanguageManager::instance();
    languageMgr.init();
    // 初始化多语言管理器的Action
    setupLanguageAction();

    // 加载样式表
    QString styleSheet = Ui::loadStyleSheet("./setting/styles/barcode_widget.qss");
    if (!styleSheet.isEmpty()) {
        this->setStyleSheet(styleSheet);
    }
}

void BarcodeWidget::updateButtonStates() const {
    saveButton->setEnabled(false);

    if (lastSelectedFiles.isEmpty()) {
        generateButton->setEnabled(false);
        decodeToChemFile->setEnabled(false);
    } else {
        if (lastSelectedFiles.size() == 1) {
            auto &file = lastSelectedFiles.front();

            bool isImage = fileExtensionRegex_image.match(file).hasMatch();
            generateButton->setEnabled(!isImage);
            decodeToChemFile->setEnabled(isImage);
        } else {
            generateButton->setEnabled(true);
            decodeToChemFile->setEnabled(true);
        }
    }
}

void BarcodeWidget::onBrowseFile() const {
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setOption(QFileDialog::ShowDirsOnly, false);
    fileDialog->setWindowTitle(tr("选择需要转换的文件或图片"));
    fileDialog->open();
}

void BarcodeWidget::onGenerateClicked() {
    const auto reqWidth = widthInput->text().toInt();
    const auto reqHeight = heightInput->text().toInt();
    const auto useBase64 = base64CheckAcion->isChecked();
    const auto format = currentBarcodeFormat;

    if (directTextAction->isChecked()) {
        QString rawText = filePathEdit->text();
        if (rawText.isEmpty()) {
            return;
        }

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

            convert::result_data_entry operator()(const QString &textInput) const {
                convert::result_data_entry res;
                // 对于直接文本模式，source_file_name 可以设为空，或者设为一个标识字符串
                // 这样在保存文件时，会默认生成 "qrcode.png" 之类的名字
                res.source_file_name = "raw_text_input";

                try {
                    std::string content;
                    if (useBase64) {
                        // 如果勾选了 Base64，先将输入文本转为 UTF-8 字节流，再 Base64 编码
                        QByteArray data = textInput.toUtf8();
                        content =
                            SimpleBase64::encode(reinterpret_cast<const std::uint8_t *>(data.constData()), data.size());
                    } else {
                        content = textInput.toStdString();
                    }

                    auto img = convert::byte_to_QRCode_qimage(content, config);

                    if (!img.isNull()) {
                        res.data = img;
                    } else {
                        res.data = QString(tr("生成图片失败")).toStdString();
                    }
                } catch (const std::exception &e) { res.data.emplace<std::string>(e.what()); }
                return res;
            }
        };

        auto *watcher = new QFutureWatcher<convert::result_data_entry>(this);
        connect(watcher, &QFutureWatcher<convert::result_data_entry>::finished, [this, watcher] {
            onBatchFinish(*watcher);
        });

        // 启动异步任务
        watcher->setFuture(QtConcurrent::mapped(inputs,
                                                TextWorker{
                                                    useBase64, {reqWidth, reqHeight, format}
        }));

        return; // 结束函数，不再执行下方的文件处理逻辑
    }

    QStringList filePaths;
    filePaths.reserve(static_cast<int>(lastResults.size()));

    //因为先前的逻辑是不是图片就算文本，所以先这样吧
    std::ranges::copy(lastSelectedFiles | std::views::filter([](const QString &file) {
                          return !fileExtensionRegex_image.match(file).hasMatch();
                      }),
                      std::back_inserter(filePaths));
    if (filePaths.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("无可处理文件"));
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

        convert::result_data_entry operator()(const QString &filePath) const {
            try {
                QFile file(filePath);

                convert::result_data_entry res;
                if (!file.open(QIODevice::ReadOnly)) {
                    res.data = QString(tr("无法打开文件: ")).toStdString() + filePath.toStdString();
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
                    text = SimpleBase64::encode(reinterpret_cast<const std::uint8_t *>(data.constData()), data.size());
                } else {
                    text = data.toStdString();
                }

                auto img = convert::byte_to_QRCode_qimage(
                    text, {.target_width = reqWidth, .target_height = reqHeight, .format = format, .margin = 1});

                if (!img.isNull()) {
                    res.data = img;
                } else {
                    res.data = QString(tr("生成图片失败")).toStdString();
                }

                return res;
            } catch (const std::exception &e) {
                convert::result_data_entry res;
                res.source_file_name = std::move(filePath);
                res.data.emplace<std::string>(e.what());
                return res;
            }
        }
    };

    auto *watcher = new QFutureWatcher<convert::result_data_entry>(this);

    connect(watcher,
            &QFutureWatcher<convert::result_data_entry>::progressValueChanged,
            progressBar,
            &QProgressBar::setValue);

    connect(
        watcher, &QFutureWatcher<convert::result_data_entry>::finished, [this, watcher] { onBatchFinish(*watcher); });

    watcher->setFuture(QtConcurrent::mapped(filePaths, worker{reqWidth, reqHeight, useBase64, format}));
}

void BarcodeWidget::onDecodeToChemFileClicked() {
    const QStringList filePaths = lastSelectedFiles.filter(fileExtensionRegex_image);
    if (filePaths.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("无可处理文件"));
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
                const auto file_path = path.toLocal8Bit().toStdString();
                switch (auto rst = convert::QRcode_to_byte(file_path); rst.err) {
                case convert::result_i2t::empty_img:
                    spdlog::error("cv::imread 无法加载图片文件: {}", path.toStdString());
                    return {std::move(path), QString{tr("无法加载图片文件: %1")}.arg(path).toStdString()};
                case convert::result_i2t::invalid_qrcode:
                    return {std::move(path), QString{tr("无法识别条码或条码格式不正确")}.toStdString()};
                default:
                    std::vector<std::uint8_t> decodedData;
                    if (useBase64) {
                        decodedData = SimpleBase64::decode(rst.text);
                    } else {
                        decodedData = std::vector<std::uint8_t>(rst.text.begin(), rst.text.end());
                    }
                    return {std::move(path),
                            QByteArray(reinterpret_cast<const char *>(decodedData.data()),
                                       static_cast<int>(decodedData.size()))};
                }
            } catch (const std::exception &e) {
                return {std::move(path), QString("解码失败:\n%1").arg(e.what()).toStdString()};
            }
        }
    };

    auto *watcher = new QFutureWatcher<convert::result_data_entry>(this);

    connect(watcher,
            &QFutureWatcher<convert::result_data_entry>::progressValueChanged,
            progressBar,
            &QProgressBar::setValue);

    connect(
        watcher, &QFutureWatcher<convert::result_data_entry>::finished, [this, watcher] { onBatchFinish(*watcher); });

    watcher->setFuture(QtConcurrent::mapped(filePaths, worker{base64CheckAcion->isChecked()}));
}

void BarcodeWidget::onSaveClicked() {
    if (lastResults.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("没有可保存的内容。"));
        return;
    }

    struct SaveTask {
        convert::result_data_entry entry;
        QString dest;
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
        const auto &entry = lastResults.front();

        const QString defName = entry.get_default_target_name();
        auto fileName = std::visit<QString>(
            overload_def_noop{std::in_place_type<QString>,
                              [&](const QImage &) {
                                  return QFileDialog::getSaveFileName(
                                      this, tr("保存图片"), defName, "PNG Images (*.png)");
                              },
                              [&](const QByteArray &) {
                                  return QFileDialog::getSaveFileName(
                                      this, tr("保存文件"), defName, "Binary Files (*.rfa);;Text Files (*.txt)");
                              }},
            entry.data);

        if (fileName.isEmpty()) {
            return;
        }
        tasks.append({entry, std::move(fileName)});
    } else {
        const QString dir =
            QFileDialog::getExistingDirectory(this,
                                              tr("请选择保存文件夹"),
                                              QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (dir.isEmpty()) {
            return;
        }

        const QDir outputDir(dir);
        for (const auto &entry : lastResults) {
            if (!entry) {
                continue;
            }

            const QString fileName = outputDir.filePath(entry.get_default_target_name());
            tasks.append({entry, std::move(fileName)});
        }
    }

    if (tasks.isEmpty()) {
        return;
    }

    progressBar->setVisible(true);
    progressBar->setRange(0, tasks.size());
    progressBar->setValue(0);
    saveButton->setEnabled(false);
    generateButton->setEnabled(false);
    decodeToChemFile->setEnabled(false);
    this->setCursor(Qt::WaitCursor);

    struct worker {
        using result_type = SaveResult;
        SaveResult operator()(const SaveTask &task) const noexcept try {
            return std::visit<SaveResult>(
                overload_def_noop{std::in_place_type<SaveResult>,
                                  [&](const QImage &img) -> SaveResult {
                                      if (img.isNull()) {
                                          return {SaveResult::invalid_data, task.dest};
                                      }
                                      if (img.save(task.dest)) {
                                          return {SaveResult::success, task.dest};
                                      } else {
                                          return {SaveResult::failed, task.dest};
                                      }
                                  },
                                  [&](const QByteArray &data) -> SaveResult {
                                      if (data.isEmpty()) {
                                          return {SaveResult::invalid_data, task.dest};
                                      }

                                      QFile f(task.dest);
                                      if (f.open(QIODevice::WriteOnly)) {
                                          f.write(data);
                                          f.close();
                                          return {SaveResult::success, task.dest};
                                      }
                                      return {SaveResult::failed, task.dest};
                                  },
                                  [&](const auto &) noexcept { return SaveResult{SaveResult::failed, task.dest}; }},
                task.entry.data);
        } catch (...) { return {SaveResult::failed}; }
    };

    auto *watcher = new QFutureWatcher<SaveResult>(this);

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

        for (const auto &res : list) {
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
                case SaveResult::invalid_data: reason = tr("数据为空或无效"); break;
                case SaveResult::failed: reason = tr("写入失败"); break;
                default: reason = tr("未知错误"); break;
                }
                failedInfos.append(QString("• %1 (%2)").arg(fileName, reason));
            }
        }

        // 构建消息框内容
        QString msg = QString(tr("操作完成。\n总计处理: %1\n成功: %2\n失败: %3"))
                          .arg(list.size())
                          .arg(successCount)
                          .arg(failedInfos.size());

        if (!failedInfos.isEmpty()) {
            msg += tr("\n\n[保存失败的文件]:\n");
            // 限制显示的错误行数，防止弹窗过高
            static constexpr int maxErrorsToShow = 10;
            for (int i = 0; i < std::min(failedInfos.size(), maxErrorsToShow); ++i) {
                msg += failedInfos[i] + "\n";
            }
            if (failedInfos.size() > maxErrorsToShow) {
                msg += QString(tr("...以及其他 %1 个文件")).arg(failedInfos.size() - maxErrorsToShow);
            }
            QMessageBox::warning(this, tr("保存结果 - 包含错误"), msg);
        } else {
            // 全部成功的情况
            if (!successInfos.isEmpty() && list.size() > 1) {
                msg += tr("\n\n[文件列表]:\n") + successInfos.join("\n");
            }
            QMessageBox::information(this, tr("保存成功"), msg);
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
    const QString system_version = version::system_version.data();
    const QString kernel_version = version::kernel_version.data();
    const QString architecture = version::architecture.data();

    AboutDialog *aboutDialog = new AboutDialog();
    aboutDialog->setVersionInfo(tag, hash, branch, commitTime, buildTime, system_version, kernel_version, architecture);
    aboutDialog->exec();
    aboutDialog->deleteLater();
}

void BarcodeWidget::showMqttDebugMonitor() const {
    messageWidget->show();
}

QImage BarcodeWidget::MatToQImage(const cv::Mat &mat) const {
    if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8).copy();
    } else if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }
    return {};
}

void BarcodeWidget::renderResults() const {
    QWidget *container = new QWidget();
    container->setObjectName("resultContainer");

    if (lastResults.empty() && directTextAction->isChecked()) {
        QVBoxLayout *vLayout = new QVBoxLayout(container);
        QLabel *infoLabel = new QLabel(tr("当前模式：直接文本生成\n请输入内容并点击生成"));
        infoLabel->setObjectName("infoLabel");
        infoLabel->setAlignment(Qt::AlignCenter);
        vLayout->addWidget(infoLabel);
        scrollArea->setWidget(container);
        return;
    }

    constexpr int maxColumns = 4; // 每行最多4个 (仅用于多结果)

    if (lastResults.empty()) {
        if (!lastSelectedFiles.empty()) {
            QVBoxLayout *listLayout = new QVBoxLayout(container);
            listLayout->setContentsMargins(10, 10, 10, 10);
            listLayout->setAlignment(Qt::AlignTop);

            QLabel *headerLabel = new QLabel(QString(tr("已选择 %1 个文件，准备处理:")).arg(lastSelectedFiles.size()));
            headerLabel->setObjectName("headerLabel");
            listLayout->addWidget(headerLabel);

            for (const QString &filePath : lastSelectedFiles) {
                QFileInfo fi(filePath);
                QString fileName = fi.fileName();

                // 判断文件类型以决定图标和操作提示
                bool isText = fileExtensionRegex_text.match(fileName).hasMatch();
                bool isImage = fileExtensionRegex_image.match(fileName).hasMatch();

                // 创建单行容器 Widget
                QWidget *rowWidget = new QWidget();
                rowWidget->setObjectName("fileRowWidget");

                QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
                rowLayout->setContentsMargins(10, 8, 10, 8);
                rowLayout->setSpacing(12);

                // 1. 图标 Label
                QLabel *iconLabel = new QLabel();
                iconLabel->setObjectName("fileIconLabel");
                iconLabel->setFixedSize(24, 24);

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
                QLabel *nameLabel = new QLabel(fileName);
                nameLabel->setObjectName("fileNameLabel");
                nameLabel->setToolTip(filePath); // 鼠标悬停显示完整路径

                // 4. 类型/操作提示标签
                QLabel *typeLabel = new QLabel();
                typeLabel->setObjectName("fileTypeLabel");

                if (isImage) {
                    typeLabel->setText(tr("[待解码]"));
                    typeLabel->setProperty("status", "decode");
                } else if (isText) {
                    typeLabel->setText(tr("[待生成]"));
                    typeLabel->setProperty("status", "generate");
                } else {
                    typeLabel->setText(tr("[不确定类型，默认待生成]"));
                    typeLabel->setProperty("status", "unknown");
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
            QVBoxLayout *vLayout = new QVBoxLayout(container);
            QLabel *emptyLabel = new QLabel(tr("请选择文件\n或者键入内容"));
            emptyLabel->setObjectName("emptyLabel");
            emptyLabel->setAlignment(Qt::AlignCenter);
            vLayout->addWidget(emptyLabel);
        }
    } else if (lastResults.size() == 1) {
        // --- 单个结果展示逻辑 (保持原样) ---
        const auto &entry = lastResults.front();
        QVBoxLayout *singleLayout = new QVBoxLayout(container);
        singleLayout->setContentsMargins(10, 10, 10, 10);
        singleLayout->setSpacing(5);

        QWidget *contentWidget = nullptr;

        std::visit(overload_def_noop{
                       std::in_place_type<void>,
                       [&](const QImage &img) {
                           QLabel *imgLabel = new QLabel();
                           imgLabel->setObjectName("imageLabel");
                           imgLabel->setPixmap(QPixmap::fromImage(img));
                           imgLabel->setAlignment(Qt::AlignCenter);
                           imgLabel->setToolTip(QString("Size: %1x%2").arg(img.width()).arg(img.height()));

                           // [修改] 将最小尺寸设置为图片尺寸，确保大图能撑开 ScrollArea 出现滚动条
                           imgLabel->setMinimumSize(img.size());

                           contentWidget = imgLabel;
                       },
                       [&](const QByteArray &data) {
                           QLabel *textLabel = new QLabel();
                           textLabel->setObjectName("textLabel");
                           // 显示完整解码内容
                           QString textDisplay = QString::fromUtf8(data);

                           textLabel->setText(textDisplay);
                           textLabel->setWordWrap(true);

                           textLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                           textLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // 允许内容撑开
                           contentWidget = textLabel;
                       },
                       [&](const std::string &str) {
                           QLabel *errLabel = new QLabel(QString::fromStdString(str));
                           errLabel->setObjectName("errorLabel");
                           errLabel->setWordWrap(true);

                           errLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                           errLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                           contentWidget = errLabel;
                       }},
                   entry.data);

        if (contentWidget) {
            singleLayout->addWidget(contentWidget, 1); // 权重设为 1 占据空间
        }
    } else {
        // --- 多个结果网格展示逻辑 (保持原样) ---
        QGridLayout *gridLayout = new QGridLayout(container);
        gridLayout->setHorizontalSpacing(20);
        gridLayout->setVerticalSpacing(40);
        gridLayout->setContentsMargins(20, 20, 20, 20);

        int count = 0;

        for (const auto &entry : lastResults) {
            int row = count / maxColumns;
            int col = count % maxColumns;

            QWidget *cellWidget = new QWidget();
            QVBoxLayout *cellLayout = new QVBoxLayout(cellWidget);
            cellLayout->setContentsMargins(0, 0, 0, 0);
            cellLayout->setSpacing(5);

            QWidget *contentWidget = nullptr;

            std::visit(overload_def_noop{std::in_place_type<void>,
                                         // 图片类型，显示缩略图
                                         [&](const QImage &img) {
                                             QLabel *imgLabel = new QLabel();
                                             imgLabel->setObjectName("imageLabel");
                                             // 使用缩略图大小 200x200
                                             imgLabel->setPixmap(QPixmap::fromImage(img).scaled(
                                                 200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                                             imgLabel->setAlignment(Qt::AlignCenter);
                                             imgLabel->setToolTip(
                                                 QString("Size: %1x%2").arg(img.width()).arg(img.height()));
                                             contentWidget = imgLabel;
                                         },
                                         // 文本类型，显示前200字符 (截断)
                                         [&](const QByteArray &data) {
                                             QLabel *textLabel = new QLabel();
                                             textLabel->setObjectName("gridTextLabel");
                                             QString textDisplay = QString::fromUtf8(data);
                                             if (textDisplay.length() > 256) {
                                                 textDisplay = textDisplay.left(256) + "...";
                                             }
                                             textLabel->setText(textDisplay);
                                             textLabel->setWordWrap(true);

                                             textLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                                             textLabel->setFixedSize(200, 200);
                                             contentWidget = textLabel;
                                         },
                                         // 错误信息，显示解码或生成的错误内容
                                         [&](const std::string &str) {
                                             QLabel *errLabel = new QLabel(QString::fromStdString(str));
                                             errLabel->setObjectName("gridErrorLabel");
                                             errLabel->setWordWrap(true);

                                             errLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

                                             errLabel->setFixedSize(200, 200);
                                             contentWidget = errLabel;
                                         }},
                       entry.data);

            if (contentWidget) {
                // 仅在多结果模式下渲染文件名
                QString fileNameStr = QFileInfo(entry.source_file_name).fileName();
                if (fileNameStr.isEmpty()) {
                    fileNameStr = "Unknown";
                }

                QLabel *nameLabel = new QLabel(fileNameStr);
                nameLabel->setObjectName("resultNameLabel");
                nameLabel->setAlignment(Qt::AlignCenter);
                nameLabel->setFixedWidth(200);
                nameLabel->setToolTip(entry.source_file_name);

                QFontMetrics metrics(nameLabel->font());
                QString elidedText = metrics.elidedText(fileNameStr, Qt::ElideMiddle, 200);
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

void BarcodeWidget::onBatchFinish(QFutureWatcher<convert::result_data_entry> &watcher) {
    setCursor(Qt::ArrowCursor);
    if (lastSelectedFiles.size() == 1) {
        auto &file = lastSelectedFiles.front();
        bool isImage = fileExtensionRegex_image.match(file).hasMatch();
        generateButton->setEnabled(!isImage);
        decodeToChemFile->setEnabled(isImage);
    } else if (!lastSelectedFiles.isEmpty()) {
        generateButton->setEnabled(true);
        decodeToChemFile->setEnabled(true);
    }

    progressBar->setVisible(false);

    QList<convert::result_data_entry> results = watcher.future().results();

    lastResults.clear();
    lastResults.reserve(results.size());
    for (auto &item : results) {
        lastResults.push_back(std::move(item));
    }

    if (!lastResults.empty()) {
        saveButton->setEnabled(true);
        renderResults(); // 批量渲染结果
    }

    watcher.deleteLater();
}

template <>
struct magic_enum::customize::enum_range<ZXing::BarcodeFormat> {
    static constexpr bool is_flags = true;
};

QString BarcodeWidget::barcodeFormatToString(ZXing::BarcodeFormat format) {
    static const auto map = [] {
        QMap<ZXing::BarcodeFormat, QString> map;
        for (const auto &[k, v] : magic_enum::enum_entries<ZXing::BarcodeFormat>()) {
            map.insert(k, QString::fromUtf8(v.data(), static_cast<int>(v.size())));
        }
        return map;
    }();

    return map.value(format, "Unknown");
}

ZXing::BarcodeFormat BarcodeWidget::stringToBarcodeFormat(const QString &formatStr) {
    static const auto map = [] {
        QMap<QString, ZXing::BarcodeFormat> map;
        for (const auto &[k, v] : magic_enum::enum_entries<ZXing::BarcodeFormat>()) {
            map.insert(QString::fromUtf8(v.data(), static_cast<int>(v.size())), k);
        }
        return map;
    }();

    QString key = formatStr.trimmed();
    key[0] = key[0].toUpper(); // 确保首字母大写以匹配上面的key

    return map.value(key, ZXing::BarcodeFormat::None); // 未匹配时返回None
}

void BarcodeWidget::setupLanguageAction() {
    LanguageManager &languageMgr = LanguageManager::instance();

    QActionGroup *langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    const QStringList displayNames = languageMgr.availableDisPlayNames();

    for (const QString &displayName : displayNames) {
        QString localeKey = languageMgr.localeFromDisplayName(displayName);
        if (localeKey.isEmpty()) {
            continue;
        }

        QAction *action = new QAction(displayName, this);
        action->setCheckable(true);
        action->setChecked(languageMgr.currentLocale() == localeKey);
        action->setData(localeKey);

        languageSubMenu->addAction(action);
        langGroup->addAction(action);
    }

    connect(langGroup, &QActionGroup::triggered, this, [&languageMgr](QAction *action) {
        QString locale = action->data().toString();
        languageMgr.switchLanguage(locale);
    });
}

void BarcodeWidget::retranslate() {
    helpMenu->setTitle(tr("帮助"));
    toolsMenu->setTitle(tr("工具"));
    settingMenu->setTitle(tr("设置"));
    languageSubMenu->setTitle(tr("语言"));
    aboutAction->setText(tr("关于软件"));
    debugMqttAction->setText(tr("MQTT实时消息监控窗口"));
    openCameraScanAction->setText(tr("打开摄像头扫码"));
    base64CheckAcion->setText(tr("Base64"));
    directTextAction->setText(tr("文本输入"));
    filePathEdit->setPlaceholderText(tr("选择一个文件或图片"));
    browseButton->setText(tr("浏览"));
    generateButton->setText(tr("生成"));
    decodeToChemFile->setText(tr("解码"));
    saveButton->setText(tr("保存"));
    generateButton->setToolTip(tr("请选择任意文件来生成条码"));
    decodeToChemFile->setToolTip(tr("可以解码PNG图片中的条码"));
    formatLabel->setText(tr("选择条码类型:"));
    widthLabel->setText(tr("宽度:"));
    heightLabel->setText(tr("高度:"));
    // 调用该函数触发正确的显示
    renderResults();
}

void BarcodeWidget::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}

const QStringList BarcodeWidget::barcodeFormats = [] {
    QStringList list;
    for (const auto &[k, v] : magic_enum::enum_entries<ZXing::BarcodeFormat>()) {
        list.append(QString::fromUtf8(v.data(), static_cast<int>(v.size())));
    }
    return list;
}();