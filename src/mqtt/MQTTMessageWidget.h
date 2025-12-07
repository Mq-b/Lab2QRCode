#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDateTime>
#include <QScrollBar>
#include <QHeaderView>

/**
 * @class MQTTMessageWidget
 * @brief 用于显示 MQTT 消息的组件
 */
class MQTTMessageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MQTTMessageWidget(QWidget* parent = nullptr);
    void addMessage(const QString& topic, const QByteArray& rawData) const;

public slots:
    void clearMessages();
    void exportMessages();

private:
    void setupUI();

    QTabWidget* tabWidget;
    QTableWidget* tableWidget;
    QTextEdit* rawTextView;
    QPushButton* clearButton;
    QPushButton* exportButton;
    QLabel* statusLabel;
};