#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include "CameraConfig.h"
#include "FrameWidget.h"
#include "commondef.h"
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <ZXing/BarcodeFormat.h>
#include <atomic>
#include <future>
#include <opencv2/opencv.hpp>
#include <qactiongroup.h>
#include <qcombobox.h>
#include <thread>

class QHideEvent;
class QPushButton;
class QMenuBar;
class QLabel;
class QTimer;
class QTableView;
class QStandardItemModel;

/**
 * @class CameraWidget
 * @brief 摄像头预览和二维码扫描组件
 * 
 * 该组件负责打开摄像头设备，捕获视频流，并在视频中实时检测条码。
 * 检测到的条码会在视频预览中用绿色方框标出，并在下方表格中显示解码结果。
 */
class CameraWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化摄像头预览窗口
     * 
     * @param parent 父窗口指针，默认为空
     */
    explicit CameraWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数，释放摄像头资源
     */
    ~CameraWidget();

    /**
     * @brief 启动摄像头设备
     * 
     * @param camIndex 摄像头设备索引，默认为0
     */
    void startCamera(int camIndex = 0);

    /**
     * @brief 停止摄像头设备并释放相关资源
     */
    void stopCamera();

protected:
    /**
     * @brief 事件过滤器函数
     * 
     * @param obj 事件目标对象
     * @param event 事件对象
     * @return 如果事件被处理则返回 true，否则返回 false
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief 窗口隐藏事件处理函数
     * 
     * 当窗口被隐藏时自动停止摄像头捕获
     * @param event 隐藏事件对象
     */
    void hideEvent(QHideEvent *event) override;

    /**
     * @brief 窗口显示事件处理函数
     * 
     * 当窗口重新显示时自动启动摄像头捕获
     * @param event 显示事件对象
     */
    void showEvent(QShowEvent *event) override;

private:
    /**
     * @brief 摄像头设备切换处理函数
     * 
     * 当用户选择不同摄像头设备时调用此函数进行切换
     * @param index 新选择的摄像头设备索引
     */
    void onCameraIndexChanged(int index);

    /**
     * @brief 更新视频帧显示
     * 
     * 在UI线程中更新视频帧显示，并处理条码识别结果
     * @param r 视频帧处理结果
     */
    void updateFrame(const FrameResult &r) const;

    /**
     * @brief 导出扫描结果为 HTML 文件
     */
    bool exportResultsToHtml(const QString &filePath);
    /**
     * @brief 导出扫描结果为 XLSX 文件
     */
    bool exportResultsToXlsx(const QString &filePath);

    /**
     * @brief 摄像头捕获循环函数
     * 
     * 在独立线程中持续捕获摄像头视频帧并进行处理
     */
    void captureLoop();

    /**
     * @brief 处理视频帧中的条码识别
     * 
     * 对输入的视频帧进行条码识别，并在找到条码时进行标记
     * @param frame 输入的视频帧（会被修改以绘制条码标记）
     * @param out 识别结果输出参数
     */
    void processFrame(cv::Mat &frame, FrameResult &out) const;

    /**
     * @brief 摄像头配置切换处理函数
     *
     * 当用户选择摄像头不同的配置时调用此函数进行切换
     * @param config 新选择的摄像头配置
     */
    void onCameraConfigSelected(CameraConfig config);

    /**
     * @brief 摄像头配置 UI 加载函数
     *
     * 根据新的配置文件重新绘制 UI 下拉菜单
     * @param configs 新选择的配置
     */
    void loadCameraConfigs(const std::vector<CameraConfig> &configs);

    /**
     * @brief UI 勾选最佳配置函数
     *
     * 在配置菜单勾选最佳配置
     * @param bestConfig 最佳配置
     */
    void selectBestCameraConfigUI(const CameraConfig &bestConfig) const;

    /**
     * @brief 保存调试帧
     * 
     * 当调试模式开启且识别到条码时，保存当前帧到 debug_frames 目录
     * @param r 识别结果
     */
    void saveDebugFrame(const FrameResult &r) const;

    /**
     * @brief 根据表格第一行内容更新静态成员变量 lastContent 和 lastType
     * 
     * 在删除结果时进行调用
     */
    void updateLastFromModel();

private:
    cv::VideoCapture *capture = nullptr;       /**< 摄像头捕获对象，用于获取视频帧 */
    std::atomic_bool running{false};           /**< 控制摄像头捕获循环是否运行的原子布尔值 */
    std::thread captureThread;                 /**< 摄像头捕获线程对象 */
    std::future<void> asyncOpenFuture;         /**< 异步打开摄像头的 future 对象 */
    bool cameraStarted = false;                /**< 标记摄像头是否已经启动 */
    std::atomic_bool isEnabledScan = true;     /**< 控制是否启用条码扫描功能的原子布尔值 */
    QVBoxLayout *mainLayout = nullptr;         /**< 主布局管理器 */
    FrameWidget *frameWidget = nullptr;        /**< 视频帧显示组件 */
    QTableView *resultDisplay;                 /**< 结果显示表格视图 */
    QStandardItemModel *resultModel;           /**< 结果显示表格的数据模型 */
    QStatusBar *statusBar = nullptr;           /**< 状态栏组件 */
    QMenuBar *menuBar;                         /**< 菜单栏组件 */
    QMenu *cameraMenu;                         /**< 摄像头选择菜单 */
    QMenu *cameraConfigMenu;                   /**< 摄像头配置选择菜单 */
    QActionGroup *cameraActionGroup = nullptr; /**< 摄像头配置ActionGroup */
    int currentCameraIndex = 0;                /**< 当前选择的摄像头索引 */
    QComboBox *barcodeTypeCombo = nullptr;     /**< 条码类型选择组合框 */
    ZXing::BarcodeFormat currentBarcodeFormat = ZXing::BarcodeFormat::None; /**< 当前选择的条码格式 */
    QLabel *cameraStatusLabel;                                              /**< 摄像头状态标签 */
    QLabel *barcodeStatusLabel;                                             /**< 条码识别状态标签 */
    QTimer *barcodeClearTimer;                                              /**< 条码状态清除定时器 */
    bool isEnhanceEnabled = true;                                           /**< 是否启用图像增强 */
    bool isDebugMode = false;                                               /**< 是否启用调试模式（保存识别帧） */
    static QString lastContent;                                             /**< 用于记录上一次扫码结果内容 */
    static QString lastType;                                                /**< 用于记录上一次扫码结果类型 */
};

#endif // CAMERAWIDGET_H