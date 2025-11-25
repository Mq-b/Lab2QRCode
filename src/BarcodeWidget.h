#pragma once

#include <vector>

#include <QWidget>
#include <ZXing/BarcodeFormat.h>
#include <opencv2/opencv.hpp>
#include <qfuturewatcher.h>

#include "convert.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/MQTTMessageWidget.h"

class QLineEdit;
class QPushButton;
class QLabel;
class QScrollArea;
class QCheckBox;
class QComboBox;
class QFileDialog;
class QProgressBar;

/**
 * @class BarcodeWidget
 * @brief 该类用于实现 条码图片生成和解析功能的窗口。
 *
 * BarcodeWidget 提供了一个 GUI 界面，支持用户选择文件、生成条码、解码条码内容以及保存生成的条码图像。
 */
class BarcodeWidget : public QWidget {
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
        "QRCode",          // ZXing::BarcodeFormat::QRCode
        "Aztec",           // ZXing::BarcodeFormat::Aztec
        "Codabar",         // ZXing::BarcodeFormat::Codabar
        "Code39",          // ZXing::BarcodeFormat::Code39
        "Code93",          // ZXing::BarcodeFormat::Code93
        "Code128",         // ZXing::BarcodeFormat::Code128
        "DataBar",         // ZXing::BarcodeFormat::DataBar
        "DataBarExpanded", // ZXing::BarcodeFormat::DataBarExpanded
        "DataMatrix",      // ZXing::BarcodeFormat::DataMatrix
        "EAN8",            // ZXing::BarcodeFormat::EAN8
        "EAN13",           // ZXing::BarcodeFormat::EAN13
        "ITF",             // ZXing::BarcodeFormat::ITF
        "MaxiCode",        // ZXing::BarcodeFormat::MaxiCode
        "PDF417",          // ZXing::BarcodeFormat::PDF417
        "UPCA",            // ZXing::BarcodeFormat::UPCA
        "UPCE",            // ZXing::BarcodeFormat::UPCE
        "MicroQRCode",     // ZXing::BarcodeFormat::MicroQRCode
        "RMQRCode",        // ZXing::BarcodeFormat::RMQRCode
        "DXFilmEdge",      // ZXing::BarcodeFormat::DXFilmEdge
        "DataBarLimited"   // ZXing::BarcodeFormat::DataBarLimited
    };

signals:
    /**
     * @brief MQTT 消息接收信号
     * @param topic 接收到消息的主题(Topic)路径
     * @param payload 消息的有效载荷内容，通常为JSON格式或字符串数据
     *
     * @details
     * 当客户端订阅的 MQTT 主题有消息到达时，会触发此信号。
     */
    void mqttMessageReceived(const QString& topic, const QByteArray& payload);

private slots:
    /**
     * @brief 更新按钮状态，是否可点击
     *
     */
    void updateButtonStates() const;

    /**
     * @brief 打开文件浏览器并选择一个文件、文件夹，在文本框显示选择的路径。
     */
    void onBrowseFile() const;

    /**
     * @brief 成条码并显示。
     */
    void onGenerateClicked();

    /**
     * @brief 解码条码并显示。
     */
    void onDecodeToChemFileClicked();

    /**
     * @brief 保存当前显示的条码图像为文件。
     */
    void onSaveClicked();

    /**
     * @brief 显示关于软件的信息对话框。
     */
    void showAbout() const;

    /**
     * @brief 显示 MQTT 消息调试监控窗口。
     */
    void showMqttDebugMonitor() const;

    /**
     * @brief 将 OpenCV 中的 Mat 对象转换为 QImage 格式。
     */
    QImage MatToQImage(const cv::Mat& mat) const;

    /**
    * @brief 渲染并显示结果
    */
    void renderResults() const;

    /**
    * @brief 批处理完成回调函数
    * @param watcher 异步任务监视器
    */
    void onBatchFinish(QFutureWatcher<convert::result_data_entry>& watcher);

    /**
     * @brief 将条码格式枚举转换为字符串表示。
     *
     * @param format 条码格式枚举值。
     * @return 对应的字符串表示。
     */
    static QString barcodeFormatToString(ZXing::BarcodeFormat format);

    /**
     * @brief 将字符串表示转换为条码格式枚举。
     *
     * @param formatStr 条码格式的字符串表示。
     * @return 对应的条码格式枚举值。
     */
    static ZXing::BarcodeFormat stringToBarcodeFormat(const QString& formatStr);

private:

    QStringList lastSelectedFiles;

    QLineEdit*                              filePathEdit;        /**< 文件路径输入框，用于显示选择的文件路径 */
    QPushButton*                            generateButton;      /**< 生成条码按钮 */
    QPushButton*                            decodeToChemFile;    /**< 解码并保存为化验文件按钮 */
    QPushButton*                            saveButton;          /**< 保存条码图片按钮 */
    QProgressBar*                           progressBar;         /**< 异步动作进度条 */
    std::vector<convert::result_data_entry> lastResults;         /**< 上次解码产生的结果 */
    QScrollArea*                            scrollArea;          /**< 滚动区域 */
    QCheckBox*                              base64CheckBox;      /**< 是否使用base64 */
    QCheckBox*                              directTextCheckBox;  /**< 新增：是否直接使用文本框内容作为数据 */
    QComboBox*                              formatComboBox;      /**< 条码格式选择框 */
    ZXing::BarcodeFormat            currentBarcodeFormat = ZXing::BarcodeFormat::QRCode; /**< 当前选择的条码格式  */
    QLineEdit*                      widthInput;                                          /**< 图片宽度输入框  */
    QLineEdit*                      heightInput;                                         /**< 图片高度输入框  */
    QFileDialog*                    fileDialog;                                          /**< 文件选择弹窗    */
    std::unique_ptr<MqttSubscriber> subscriber_;                                         /**< MQTT 订阅者实例  */
    std::unique_ptr<MQTTMessageWidget> messageWidget;
};
