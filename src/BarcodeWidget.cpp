#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QPixmap>
#include <QMessageBox>
#include <QFont>
#include <QCheckBox>
#include <QComboBox>
#include <opencv2/opencv.hpp>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/BitMatrix.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/TextUtfEncoding.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/DecodeHints.h>
#include <SimpleBase64.h>
#include "BarcodeWidget.h"

QImage BarcodeWidget::MatToQImage(const cv::Mat& mat) const
{
    if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8).copy();
    else if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }
    return {};
}

BarcodeWidget::BarcodeWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Binary to QRCode Generator");
    setMinimumSize(500, 500);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);  // 调整控件之间的间距
    mainLayout->setContentsMargins(30, 20, 30, 20);  // 设置内边距

    auto* fileLayout = new QHBoxLayout();
    filePathEdit = new QLineEdit(this);
    filePathEdit->setPlaceholderText("选择一个文件或图片");
    filePathEdit->setFont(QFont("Arial", 14));
    filePathEdit->setStyleSheet("QLineEdit { border: 1px solid #ccc; border-radius: 5px; padding: 5px; background-color: #f9f9f9; }");

    QPushButton* browseButton = new QPushButton("浏览", this);
    browseButton->setFixedWidth(100);
    browseButton->setFont(QFont("Arial", 16));
    browseButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border-radius: 5px; padding: 10px; }"
        "QPushButton:disabled { background-color: #ddd; }");
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);
    mainLayout->addLayout(fileLayout);

    // 生成与保存按钮
    auto* buttonLayout = new QHBoxLayout();
    generateButton = new QPushButton("生成", this);
    decodeToChemFile = new QPushButton("解码");
    saveButton = new QPushButton("保存", this);
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

    // 图片展示区域
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(320);
    scrollArea->setStyleSheet("QScrollArea { background-color: #f0f0f0; border: 1px solid #ccc; }");

    // 图片显示内容
    barcodeLabel = new QLabel();
    barcodeLabel->setAlignment(Qt::AlignCenter);  // 图片居中
    barcodeLabel->setStyleSheet("QLabel { background-color: #fafafa; padding: 16px; font-family: Consolas; font-size: 14pt;  }");
    barcodeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 将 QLabel 设置为滚动区域的内容
    scrollArea->setWidget(barcodeLabel);

    mainLayout->addWidget(scrollArea);

    base64CheckBox = new QCheckBox("Base64", this);
    base64CheckBox->setFont(QFont("Arial", 14));
    base64CheckBox->setChecked(true);
    buttonLayout->addWidget(base64CheckBox);

    auto* comboBoxLayout = new QHBoxLayout();
    
    QComboBox* formatComboBox = new QComboBox(this);
    formatComboBox->setFont(QFont("Consolas", 13));

    for (const auto& item : qAsConst(barcodeFormats)) {
        formatComboBox->addItem(item);
    }

    QLabel* formatLabel = new QLabel("选择条码类型:", this);
    formatLabel->setFont(QFont("Consolas", 12));
    comboBoxLayout->addWidget(formatLabel);
    comboBoxLayout->addWidget(formatComboBox);
    mainLayout->addLayout(comboBoxLayout);

    // 连接信号与槽
    connect(browseButton, &QPushButton::clicked, this, &BarcodeWidget::onBrowseFile);
    connect(generateButton, &QPushButton::clicked, this, &BarcodeWidget::onGenerateClicked);
    connect(decodeToChemFile, &QPushButton::clicked, this, &BarcodeWidget::onDecodeToChemFileClicked);
    connect(saveButton, &QPushButton::clicked, this, &BarcodeWidget::onSaveClicked);
    connect(filePathEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        updateButtonStates(text);
    });
    connect(formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, formatComboBox](int index) {
        // 设置当前选择的条码格式
        currentBarcodeFormat = stringToBarcodeFormat(barcodeFormats[index]);
        //QMessageBox::information(this, "", barcodeFormatToString(currentBarcodeFormat));
    });
}


void BarcodeWidget::updateButtonStates(const QString& filePath) const
{
    if (filePath.isEmpty()) {
        // 文件路径为空，禁用所有功能按钮
        generateButton->setEnabled(false);
        decodeToChemFile->setEnabled(false);
        saveButton->setEnabled(false);
        return;
    }

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    QStringList imageFormats = { "png", "jpg", "jpeg", "bmp", "gif", "tiff", "webp" };

    if (imageFormats.contains(suffix)) {
        generateButton->setEnabled(false);
        decodeToChemFile->setEnabled(true);
        saveButton->setEnabled(false);  // 解码后才启用保存

        // 设置工具提示
        generateButton->setToolTip("请选择任意文件来生成QR码");
        decodeToChemFile->setToolTip("可以解码PNG图片中的QR码");

    }
    else {
        generateButton->setEnabled(true);
        decodeToChemFile->setEnabled(false);
        saveButton->setEnabled(false);  // 生成后才启用保存

        generateButton->setToolTip("可以从任意文件生成QR码");
        decodeToChemFile->setToolTip("请选择PNG图片来解码QR码");

    }
}

void BarcodeWidget::onBrowseFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select File", "", "Supported Files (*.rfa *.png);;All Files (*)");
    if (!fileName.isEmpty()) {
        filePathEdit->setText(fileName);
        barcodeLabel->clear();

        updateButtonStates(fileName);
    }
}

void BarcodeWidget::onGenerateClicked()
{
    QString filePath = filePathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择一个文件.");
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "不能打开文件.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    try {
        // 是否base64处理通过判断base64CheckBox
        std::string text;
        if(base64CheckBox->isChecked()){
            text = SimpleBase64::encode(reinterpret_cast<const std::uint8_t*>(data.constData()), data.size());
        }else{
            text = data.toStdString();
        }

        ZXing::MultiFormatWriter writer(currentBarcodeFormat);
        writer.setMargin(1);

        const auto bitMatrix = writer.encode(text, 300, 300);
        const int width = bitMatrix.width();
        const int height = bitMatrix.height();
        cv::Mat img(height, width, CV_8UC1);

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                img.at<uint8_t>(y, x) = bitMatrix.get(x, y) ? 0 : 255;

        lastImage = MatToQImage(img);
        barcodeLabel->clear();
        barcodeLabel->setAlignment(Qt::AlignCenter); // 重新设置居中
        barcodeLabel->setPixmap(QPixmap::fromImage(lastImage).scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        saveButton->setEnabled(true);
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to generate QRCode:\n%1").arg(e.what()));
    }
}

void BarcodeWidget::onDecodeToChemFileClicked()
{
    QString filePath = filePathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择一个PNG图片文件.");
        return;
    }

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QStringList imageFormats = { "png" };

    if (!imageFormats.contains(suffix)) {
        QMessageBox::warning(this, "警告", "选择的文件不是PNG图片格式。\n请选择300x300像素的PNG格式图片");
        return;
    }

    try {
        cv::Mat img = cv::imread(filePath.toStdString(), cv::IMREAD_COLOR);
        if (img.empty()) {
            QMessageBox::critical(this, "错误", "无法加载图片文件。");
            return;
        }

        // 转为灰度
        cv::Mat grayImg;
        cv::cvtColor(img, grayImg, cv::COLOR_BGR2GRAY);

        ZXing::ImageView imageView(grayImg.data, grayImg.cols, grayImg.rows, ZXing::ImageFormat::Lum);
        auto result = ZXing::ReadBarcode(imageView);

        if (!result.isValid()) {
            QMessageBox::warning(this, "警告", "无法识别QR码或QR码格式不正确。");
            return;
        }

        // Base64 解码
        std::string encodedText = result.text();
        std::vector<std::uint8_t> decodedData;
        if (base64CheckBox->isChecked()) {
            decodedData = SimpleBase64::decode(encodedText);
        }
        else {
            decodedData = std::vector<std::uint8_t>(encodedText.begin(), encodedText.end());
        }
        

        // ✅ 显示部分内容到界面
        QString preview;
        if (decodedData.size() > 1024)
            preview = QString::fromLatin1(reinterpret_cast<const char*>(decodedData.data()), 1024) + "\n... (内容已截断)";
        else
            preview = QString::fromLatin1(reinterpret_cast<const char*>(decodedData.data()), static_cast<int>(decodedData.size()));

        barcodeLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        barcodeLabel->setWordWrap(false);
        barcodeLabel->setText(preview);
        saveButton->setEnabled(true);

        // 保存解码后的数据到数据成员，供保存按钮使用
        lastDecodedData = QByteArray(reinterpret_cast<const char*>(decodedData.data()), static_cast<int>(decodedData.size()));
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "错误", QString("解码失败:\n%1").arg(e.what()));
    }
}

void BarcodeWidget::onSaveClicked()
{
    QString filePath = filePathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "没有可保存的内容。");
        return;
    }

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    if (suffix == "png") {
        // 如果是PNG图片，保存解码后的文件
        if (lastDecodedData.isEmpty()) {
            QMessageBox::warning(this, "警告", "没有解码数据可保存。");
            return;
        }

        QString defaultName = fileInfo.completeBaseName() + ".rfa";
        QString savePath = QFileDialog::getSaveFileName(this, "保存文件",
            fileInfo.dir().filePath(defaultName),
            "RFA Files (*.rfa)");

        if (!savePath.isEmpty()) {
            QFile outputFile(savePath);
            if (outputFile.open(QIODevice::WriteOnly)) {
                outputFile.write(lastDecodedData);
                outputFile.close();
                QMessageBox::information(this, "成功", QString("文件已保存至:\n%1").arg(savePath));
            }
            else {
                QMessageBox::critical(this, "错误", "无法保存文件。");
            }
        }
    }
    else {
        // 如果是其他文件（普通文件），保存图片
        if (lastImage.isNull()) {
            QMessageBox::warning(this, "警告", "没有图片可保存。");
            return;
        }

        QString fileNameWithoutExtension = fileInfo.baseName();
        QString fileName = QFileDialog::getSaveFileName(this, "保存图片",
            fileNameWithoutExtension + ".png", "PNG Images (*.png)");

        if (!fileName.isEmpty()) {
            if (lastImage.save(fileName))
                QMessageBox::information(this, "保存", QString("图片保存成功 %1").arg(fileName));
            else
                QMessageBox::warning(this, "错误", "无法保存图片.");
        }
    }
}

QString BarcodeWidget::barcodeFormatToString(ZXing::BarcodeFormat format)
{
    static const QMap<ZXing::BarcodeFormat, QString> map = {
        {ZXing::BarcodeFormat::None,           "None"},
        {ZXing::BarcodeFormat::Aztec,          "Aztec"},
        {ZXing::BarcodeFormat::Codabar,        "Codabar"},
        {ZXing::BarcodeFormat::Code39,         "Code39"},
        {ZXing::BarcodeFormat::Code93,         "Code93"},
        {ZXing::BarcodeFormat::Code128,        "Code128"},
        {ZXing::BarcodeFormat::DataBar,        "DataBar"},
        {ZXing::BarcodeFormat::DataBarExpanded,"DataBarExpanded"},
        {ZXing::BarcodeFormat::DataMatrix,     "DataMatrix"},
        {ZXing::BarcodeFormat::EAN8,           "EAN8"},
        {ZXing::BarcodeFormat::EAN13,          "EAN13"},
        {ZXing::BarcodeFormat::ITF,            "ITF"},
        {ZXing::BarcodeFormat::MaxiCode,       "MaxiCode"},
        {ZXing::BarcodeFormat::PDF417,         "PDF417"},
        {ZXing::BarcodeFormat::QRCode,         "QRCode"},
        {ZXing::BarcodeFormat::UPCA,           "UPCA"},
        {ZXing::BarcodeFormat::UPCE,           "UPCE"},
        {ZXing::BarcodeFormat::MicroQRCode,    "MicroQRCode"},
        {ZXing::BarcodeFormat::RMQRCode,       "RMQRCode"},
        {ZXing::BarcodeFormat::DXFilmEdge,     "DXFilmEdge"},
        {ZXing::BarcodeFormat::DataBarLimited, "DataBarLimited"}
    };

    return map.value(format, "Unknown");
}


ZXing::BarcodeFormat BarcodeWidget::stringToBarcodeFormat(const QString& formatStr)
{
    static const QMap<QString, ZXing::BarcodeFormat> map = {
        {"Aztec",           ZXing::BarcodeFormat::Aztec},
        {"Codabar",         ZXing::BarcodeFormat::Codabar},
        {"Code39",          ZXing::BarcodeFormat::Code39},
        {"Code93",          ZXing::BarcodeFormat::Code93},
        {"Code128",         ZXing::BarcodeFormat::Code128},
        {"DataBar",         ZXing::BarcodeFormat::DataBar},
        {"DataBarExpanded", ZXing::BarcodeFormat::DataBarExpanded},
        {"DataMatrix",      ZXing::BarcodeFormat::DataMatrix},
        {"EAN8",            ZXing::BarcodeFormat::EAN8},
        {"EAN13",           ZXing::BarcodeFormat::EAN13},
        {"ITF",             ZXing::BarcodeFormat::ITF},
        {"MaxiCode",        ZXing::BarcodeFormat::MaxiCode},
        {"PDF417",          ZXing::BarcodeFormat::PDF417},
        {"QRCode",          ZXing::BarcodeFormat::QRCode},
        {"UPCA",            ZXing::BarcodeFormat::UPCA},
        {"UPCE",            ZXing::BarcodeFormat::UPCE},
        {"MicroQRCode",     ZXing::BarcodeFormat::MicroQRCode},
        {"RMQRCode",        ZXing::BarcodeFormat::RMQRCode},
        {"DXFilmEdge",      ZXing::BarcodeFormat::DXFilmEdge},
        {"DataBarLimited",  ZXing::BarcodeFormat::DataBarLimited}
    };

    QString key = formatStr.trimmed();
    key[0] = key[0].toUpper(); // 确保首字母大写以匹配上面的key
    auto it = map.find(key);
    if (it != map.end())
        return it.value();

    return ZXing::BarcodeFormat::None; // 未匹配时返回None
}