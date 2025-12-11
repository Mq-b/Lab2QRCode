#pragma once
#include <QDialog>

class QLabel;
class QPushButton;

/**
 * @class MessageDialog
 * @brief 通用消息对话框
 * 
 * 提供信息提示和更新提示功能，可以自定义按钮数量。
 */
class MessageDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * @brief 构造函数，默认单按钮
     * @param parent 父窗口
     */
    explicit MessageDialog(QWidget *parent = nullptr);

    /**
     * @brief 构造函数，可自定义按钮列表
     * @param parent 父窗口
     * @param buttons 按钮文本列表
     */
    explicit MessageDialog(QWidget *parent, const QStringList &buttons);

    /**
     * @brief 显示信息对话框（单按钮）
     * @param parent 父窗口
     * @param title 对话框标题
     * @param text 对话框内容
     * @return 返回点击的按钮索引（单按钮一般返回 0）
     */
    static int information(QWidget *parent, const QString &title, const QString &text);

    /**
     * @brief 显示更新对话框（可自定义按钮）
     * @param parent 父窗口
     * @param title 对话框标题
     * @param text 对话框内容
     * @param buttons 按钮文本列表
     * @return 返回点击的按钮文本
     */
    static QString updateDialog(QWidget *parent, const QString &title, const QString &text, const QStringList &buttons);

  private:
    QLabel *m_label;         /**< 显示主要内容的标签 */
    QLabel *m_titleLabel;    /**< 对话框标题标签 */
    QPushButton *m_closeBtn; /**< 默认关闭按钮 */
    QString m_clickedButton; /**< 保存用户点击的按钮文本 */

    /**
     * @brief 内部通用执行函数
     * @param title 对话框标题
     * @param text 对话框内容
     * @param btnText 单按钮文本
     * @return 返回点击按钮索引
     */
    int execWithText(const QString &title, const QString &text, const QString &btnText = "确定");
};
