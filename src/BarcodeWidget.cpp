#include "BarcodeWidget.h"
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


QStringList getTargetPath(const QFileInfo& fileInfo, const QStringList& filters) {
    QStringList filePaths;
    if (fileInfo.isDir()) {
        QDir          srcDir(fileInfo.filePath());
        QFileInfoList fileInfos = srcDir.entryInfoList(filters, QDir::Files);
        for (const auto& fi : fileInfos) {
            filePaths << fi.absoluteFilePath();
        }
    } else {
        filePaths << fileInfo.filePath();
    }
    return filePaths;
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
    QWidget*     container  = new QWidget();
    QGridLayout* gridLayout = new QGridLayout(container);

    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(40);
    gridLayout->setContentsMargins(20, 20, 20, 20);

    int           count      = 0;
    // TODO 依照分辨率决定？
    constexpr int maxColumns = 4; // 每行最多4个

    for (const auto& entry : lastResults) {
        int row                 = count / maxColumns;
        int col                 = count % maxColumns;

        QWidget*     cellWidget = new QWidget();
        QVBoxLayout* cellLayout = new QVBoxLayout(cellWidget);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(5);

        QWidget* contentWidget = nullptr;

        std::visit(overload_def_noop{std::in_place_type<void>,
                       [&](const QImage& img) {
                           QLabel* imgLabel = new QLabel();
                           imgLabel->setPixmap(QPixmap::fromImage(img).scaled(
                               200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // 缩略图大小
                           imgLabel->setAlignment(Qt::AlignCenter);                       // 图片保持居中显示
                           imgLabel->setStyleSheet("border: 1px solid #ddd; background: white;");
                           imgLabel->setToolTip(QString("Size: %1x%2").arg(img.width()).arg(img.height()));
                           contentWidget = imgLabel;
                       },
                       [&](const QByteArray& data) {
                           QLabel* textLabel   = new QLabel();
                           QString textDisplay = QString::fromUtf8(data);
                           if (textDisplay.length() > 200) {
                               textDisplay = textDisplay.left(200) + "...";
                           }
                           textLabel->setText(textDisplay);
                           textLabel->setWordWrap(true);

                           textLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

                           textLabel->setStyleSheet(
                               "border: 1px solid #ddd; background: white; padding: 5px; font-family: Consolas;");
                           textLabel->setFixedSize(200, 200);
                           contentWidget = textLabel;
                       },
                       [&](const std::string& str) {
                           QLabel* errLabel = new QLabel(QString::fromStdString(str));
                           errLabel->setStyleSheet(
                               "color: red; border: 1px solid red; background: #fff0f0; padding: 5px;");
                           errLabel->setWordWrap(true);

                           errLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

                           errLabel->setFixedSize(200, 200);
                           contentWidget = errLabel;
                       }},
            entry.data);

        if (contentWidget) {
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

    if (lastResults.empty()) {
        QLabel* emptyLabel = new QLabel("N/A");
        emptyLabel->setAlignment(Qt::AlignCenter);
        gridLayout->addWidget(emptyLabel, 0, 0);
    }

    scrollArea->setWidget(container);
}

void BarcodeWidget::onBatchFinish(QFutureWatcher<convert::result_data_entry>& watcher) {
    setCursor(Qt::ArrowCursor);
    generateButton->setEnabled(true);
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

BarcodeWidget::BarcodeWidget(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Lab2QRCode");
    setMinimumSize(500, 600);

    QMenuBar* menuBar  = new QMenuBar();
    QMenu*    helpMenu = menuBar->addMenu("帮助");

    QFont menuFont("SimHei", 12);
    menuBar->setFont(menuFont);

    QAction* aboutAction = new QAction("关于软件", this);
    aboutAction->setFont(menuFont);
    helpMenu->addAction(aboutAction);

    // 连接菜单项的点击信号
    connect(aboutAction, &QAction::triggered, this, &BarcodeWidget::showAbout);


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

    enableBatchCheckBox = new QCheckBox("批处理", this);
    enableBatchCheckBox->setToolTip("勾选后，点击浏览将选择文件夹，并自动进入批量处理模式");
    enableBatchCheckBox->setFont(QFont("Arial", 10));

    QPushButton* browseButton = new QPushButton("浏览", this);
    browseButton->setFixedWidth(100);
    browseButton->setFont(QFont("Arial", 16));
    browseButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border-radius: 5px; padding: 10px; }"
        "QPushButton:disabled { background-color: #ddd; }");
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);
    fileLayout->addWidget(enableBatchCheckBox);
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

    MqttConfig config = MqttSubscriber::load_config("./setting/config.json");

    subscriber_       = std::make_unique<MqttSubscriber>(
        config.host, config.port, config.client_id, [this](const std::string& topic, const std::string& payload) {
            emit mqttMessageReceived(QString::fromStdString(topic), QString::fromStdString(payload));
        });
    subscriber_->subscribe("test/topic");

    connect(browseButton, &QPushButton::clicked, this, &BarcodeWidget::onBrowseFile);
    connect(generateButton, &QPushButton::clicked, this, &BarcodeWidget::onGenerateClicked);
    connect(decodeToChemFile, &QPushButton::clicked, this, &BarcodeWidget::onDecodeToChemFileClicked);
    connect(saveButton, &QPushButton::clicked, this, &BarcodeWidget::onSaveClicked);
    connect(filePathEdit, &QLineEdit::textChanged, this, [this](const QString& text) { updateButtonStates(text); });

    connect(formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, formatComboBox](int index) {
        // 设置当前选择的条码格式
        currentBarcodeFormat = stringToBarcodeFormat(barcodeFormats[index]);
    });
    connect(this, &BarcodeWidget::mqttMessageReceived, this, [this](const QString& topic, const QString& payload) {
        QMessageBox::information(this, "订阅消息", QString("主题: %1\n内容: %2").arg(topic, payload));
    });
    connect(fileDialog, &QFileDialog::fileSelected, this, [this](const QString& fileName) {
        if (!fileName.isEmpty()) {
            filePathEdit->setText(fileName);
            lastResults.clear(); // 清空数据
            renderResults();     // 重新渲染（清空界面）
            updateButtonStates(fileName);
        }
    });
}


void BarcodeWidget::updateButtonStates(const QString& filePath) const {
    QFileInfo fileInfo(filePath);

    if (filePath.isEmpty()) {
        generateButton->setEnabled(false);
        decodeToChemFile->setEnabled(false);
        saveButton->setEnabled(false);
        enableBatchCheckBox->setEnabled(true);
        enableBatchCheckBox->setCheckState(Qt::CheckState::Unchecked);
        return;
    } else {
        enableBatchCheckBox->setEnabled(false);
    }


    if (fileInfo.isDir()) { // 依据实际文件类型而不是按钮状态来判断
        // 批量模式逻辑
        generateButton->setEnabled(true);
        decodeToChemFile->setEnabled(true);
        saveButton->setEnabled(false);

        generateButton->setText("批量生成");
        decodeToChemFile->setText("批量解码");

        generateButton->setToolTip("将文件夹内所有文件转换为二维码图片");
        decodeToChemFile->setToolTip("将文件夹内所有PNG图片解码为文件");

        enableBatchCheckBox->setCheckState(Qt::CheckState::Checked);
    } else {
        // 单文件模式逻辑
        generateButton->setText("生成"); // 恢复按钮文字
        decodeToChemFile->setText("解码");

        const QString     suffix       = fileInfo.suffix().toLower();
        const QStringList imageFormats = {"png", "jpg", "jpeg", "bmp", "gif", "tiff", "webp"};

        if (imageFormats.contains(suffix)) {
            generateButton->setEnabled(false);
            decodeToChemFile->setEnabled(true);
            saveButton->setEnabled(false);
            generateButton->setToolTip("请选择任意文件来生成QR码");
            decodeToChemFile->setToolTip("可以解码PNG图片中的QR码");
        } else {
            generateButton->setEnabled(true);
            decodeToChemFile->setEnabled(false);
            saveButton->setEnabled(false);
            generateButton->setToolTip("可以从任意文件生成QR码");
            decodeToChemFile->setToolTip("请选择PNG图片来解码QR码");
        }

        enableBatchCheckBox->setCheckState(Qt::CheckState::Unchecked);
    }
}

void BarcodeWidget::onBrowseFile() const {
    if (enableBatchCheckBox->isChecked()) {
        fileDialog->setFileMode(QFileDialog::Directory);
        fileDialog->setOption(QFileDialog::ShowDirsOnly, true);
        fileDialog->setWindowTitle("请选择一个包含文件的文件夹");
    } else {
        fileDialog->setFileMode(QFileDialog::ExistingFile);
        fileDialog->setOption(QFileDialog::ShowDirsOnly, false);
        fileDialog->setWindowTitle("选择一个文件或图片");
    }

    fileDialog->open();
}

void BarcodeWidget::onGenerateClicked() {
    const QString   filePath = filePathEdit->text();
    const QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "警告", "非法路径");
        return;
    }
    QStringList filters;
    filters << "*.txt" << "*.json" << "*.rfa";
    QStringList filePaths = getTargetPath(fileInfo, filters);

    if (filePaths.empty()) {
        QMessageBox::warning(this, "警告", "无可处理文件");
        return;
    }

    // 2. UI 状态准备
    progressBar->setVisible(true);
    progressBar->setRange(0, filePaths.size()); // 设置进度条范围
    progressBar->setValue(0);
    generateButton->setEnabled(false);
    saveButton->setEnabled(false);
    this->setCursor(Qt::WaitCursor);

    const auto reqWidth  = widthInput->text().toInt();
    const auto reqHeight = heightInput->text().toInt();
    const auto useBase64 = base64CheckBox->isChecked();
    const auto format    = currentBarcodeFormat;

    struct worker {
        using result_type = convert::result_data_entry;
        int                  reqWidth, reqHeight;
        bool                 useBase64;
        ZXing::BarcodeFormat format;

        convert::result_data_entry operator()(const QString& filePath) const {
            try {
                QFile file(filePath);

                convert::result_data_entry res;
                if (!file.open(QIODevice::ReadOnly)) {
                    res.data             = std::string("无法打开文件: ") + filePath.toStdString();
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

    connect(watcher, &QFutureWatcher<convert::result_data_entry>::progressValueChanged, progressBar,
        &QProgressBar::setValue);

    connect(
        watcher, &QFutureWatcher<convert::result_data_entry>::finished, [this, watcher]() { onBatchFinish(*watcher); });

    watcher->setFuture(QtConcurrent::mapped(filePaths, worker{reqWidth, reqHeight, useBase64, format}));
}

void BarcodeWidget::onDecodeToChemFileClicked() {
    const QString   filePath = filePathEdit->text();
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "警告", "非法路径");
        return;
    }
    QStringList filters;
    filters << "*.png";
    QStringList filePaths = getTargetPath(fileInfo, filters);
    if (filePaths.empty()) {
        QMessageBox::warning(this, "警告", "无可处理文件");
        return;
    }

    // 2. UI 状态准备
    progressBar->setVisible(true);
    progressBar->setRange(0, filePaths.size()); // 设置进度条范围
    progressBar->setValue(0);
    generateButton->setEnabled(false);
    saveButton->setEnabled(false);
    this->setCursor(Qt::WaitCursor);


    struct worker {
        using result_type = convert::result_data_entry;

        bool                       useBase64;
        convert::result_data_entry operator()(QString path) const {
            try {
                const auto bytes = path.toLocal8Bit().toStdString();
                switch (auto rst = convert::QRcode_to_byte(bytes); rst.err) {
                case convert::result_i2t::invalid_qrcode:
                    spdlog::error("loadImageFromFile 无法加载图片文件: {}", path.toStdString());
                    return {std::move(path), QString{"无法加载图片文件: %1"}.arg(path).toStdString()};
                case convert::result_i2t::empty_img:
                    return {std::move(path), std::string{"无法识别QR码或QR码格式不正确"}};
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

    connect(watcher, &QFutureWatcher<convert::result_data_entry>::progressValueChanged, progressBar,
        &QProgressBar::setValue);

    connect(
        watcher, &QFutureWatcher<convert::result_data_entry>::finished, [this, watcher]() { onBatchFinish(*watcher); });

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

        QDir outputDir(dir);
        for (const auto& entry : lastResults) {
            if (!entry) {
                continue;
            }

            QString fileName = outputDir.filePath(entry.get_default_target_name());
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
        updateButtonStates(filePathEdit->text());
        saveButton->setEnabled(true);
        // 保存按钮总是可以再次点击

        auto list                = watcher->future().results();

        int         successCount = 0;
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

cv::Mat BarcodeWidget::loadImageFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(nullptr, "错误", "无法打开文件");
        return {};
    }

    QByteArray fileData = file.readAll();
    file.close();

    cv::Mat img = cv::imdecode(cv::Mat(1, fileData.size(), CV_8UC1, fileData.data()), cv::IMREAD_COLOR);

    if (img.empty()) {
        QMessageBox::critical(nullptr, "cv::imdecode 错误", "无法解码图片文件。");
        return cv::Mat();
    }

    return img;
}

void BarcodeWidget::showAbout() const {
    const QString tag        = version::git_tag.data();
    const QString hash       = version::git_hash.data();
    const QString branch     = version::git_branch.data();
    const QString commitTime = version::git_commit_time.data();
    const QString buildTime  = version::build_time.data();

    AboutDialog* aboutDialog = new AboutDialog();
    aboutDialog->setVersionInfo(tag, hash, branch, commitTime, buildTime);
    aboutDialog->exec();
    aboutDialog->deleteLater();
}
