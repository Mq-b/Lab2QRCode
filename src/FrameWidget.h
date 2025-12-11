#pragma once
#include <QWidget>
#include <opencv2/core.hpp>

/**
 * @class FrameWidget
 * @brief 用于显示视频帧的自定义 QWidget
 */
class FrameWidget : public QWidget {
    Q_OBJECT
  public:
    explicit FrameWidget(QWidget *parent = nullptr);

    /**
     * @brief 设置要显示的图像帧
     *  会自动触发重绘事件
     * @param bgr 输入的 BGR 格式图像
     */
    void setFrame(const cv::Mat &bgr);

    void clear();

  protected:
    /**
     * @brief 重写绘制事件以显示视频帧
     * 
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

  private:
    QImage m_image; // 转换后的图像
};