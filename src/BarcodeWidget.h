#pragma once

#include <QWidget>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <ZXing/BarcodeFormat.h>
#include <QScrollArea>

class QLineEdit;
class QPushButton;
class QLabel;
class QCheckBox;
class QComboBox;

/**
 * @class BarcodeWidget
 * @brief 该类用于实现 QRCode 二维码图片生成和解析功能的窗口。
 *
 * BarcodeWidget 提供了一个 GUI 界面，支持用户选择文件、生成二维码、解码二维码内容以及保存生成的二维码图像。
 */
class BarcodeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数，初始化窗口和控件布局。
     *
     * @param parent 父窗口指针，默认为空指针。
     */
    explicit BarcodeWidget(QWidget* parent = nullptr);

private:

    static inline const QStringList barcodeFormats = {
        "QRCode",           // ZXing::BarcodeFormat::QRCode
        "Aztec",            // ZXing::BarcodeFormat::Aztec
        "Codabar",          // ZXing::BarcodeFormat::Codabar
        "Code39",           // ZXing::BarcodeFormat::Code39
        "Code93",           // ZXing::BarcodeFormat::Code93
        "Code128",          // ZXing::BarcodeFormat::Code128
        "DataBar",          // ZXing::BarcodeFormat::DataBar
        "DataBarExpanded",  // ZXing::BarcodeFormat::DataBarExpanded
        "DataMatrix",       // ZXing::BarcodeFormat::DataMatrix
        "EAN8",             // ZXing::BarcodeFormat::EAN8
        "EAN13",            // ZXing::BarcodeFormat::EAN13
        "ITF",              // ZXing::BarcodeFormat::ITF
        "MaxiCode",         // ZXing::BarcodeFormat::MaxiCode
        "PDF417",           // ZXing::BarcodeFormat::PDF417
        "UPCA",             // ZXing::BarcodeFormat::UPCA
        "UPCE",             // ZXing::BarcodeFormat::UPCE
        "MicroQRCode",      // ZXing::BarcodeFormat::MicroQRCode
        "RMQRCode",         // ZXing::BarcodeFormat::RMQRCode
        "DXFilmEdge",       // ZXing::BarcodeFormat::DXFilmEdge
        "DataBarLimited"    // ZXing::BarcodeFormat::DataBarLimited
    };

private slots:
    /**
     * @brief 更新按钮状态，是否可点击
     *
     * @param filePath 当前选择的文件路径。
     */
    void updateButtonStates(const QString& filePath) const;

    /**
     * @brief 打开文件浏览器并选择一个文件，在文本框显示选择的文件路径。
     */
    void onBrowseFile();

    /**
     * @brief 根据调用 onBrowseFile 选择的文件生成二维码并显示。
     */
    void onGenerateClicked();

    /**
     * @brief 解码二维码并保存为文件。
     */
    void onDecodeToChemFileClicked();

    /**
     * @brief 保存当前显示的二维码图像为文件。
     */
    void onSaveClicked();

    static QString barcodeFormatToString(ZXing::BarcodeFormat format);
    static ZXing::BarcodeFormat stringToBarcodeFormat(const QString& formatStr);

private:

    /**
     * @brief 将 OpenCV 中的 Mat 对象转换为 QImage 格式。
     *
     * @param mat 输入的 OpenCV 图像（Mat 类型）。
     * @return 转换后的 QImage 图像。
     *
     */
    QImage MatToQImage(const cv::Mat& mat) const;

    QLineEdit* filePathEdit;       /**< 文件路径输入框，用于显示选择的文件路径 */
    QPushButton* generateButton;   /**< 生成二维码按钮 */
    QPushButton* decodeToChemFile; /**< 解码并保存为化验文件按钮 */
    QPushButton* saveButton;       /**< 保存二维码图片按钮 */
    QLabel* barcodeLabel;          /**< 用于显示生成的二维码图像或解码文本的标签 */
    QImage lastImage;              /**< 存储最后生成的二维码图像 */
    QByteArray lastDecodedData;    /**< 保存解码后的数据 */
    QScrollArea* scrollArea;       /**< 滚动区域 */
    QCheckBox* base64CheckBox;     /**< 是否使用base64 */
    QComboBox* formatComboBox;     /**< 条码格式选择框 */
    ZXing::BarcodeFormat currentBarcodeFormat = ZXing::BarcodeFormat::QRCode;  /**< 当前选择的条码格式  */
};
