
#include "MQTTMessageWidget.h"
#include <QFileDialog>
#include <QTextCodec>
#include <QTextStream>

MQTTMessageWidget::MQTTMessageWidget(QWidget *parent)
    : QWidget(parent) {
    setupUI();
    setWindowTitle("MQTT消息监控");
    setMinimumSize(800, 600);
}

void MQTTMessageWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 状态栏
    statusLabel = new QLabel("就绪", this);

    // 选项卡
    tabWidget = new QTabWidget(this);

    // 表格视图
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels({"时间", "主题", "消息内容", "长度"});

    // 智能列宽设置
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // 时间列根据内容调整
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 主题列根据内容调整
    tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);          // 消息内容列拉伸填充
    tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 长度列根据内容调整

    tableWidget->setColumnWidth(0, 90); // 时间列最小宽度

    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 原始文本视图
    rawTextView = new QTextEdit(this);
    rawTextView->setReadOnly(true);
    rawTextView->setFont(QFont("Consolas", 10));

    tabWidget->addTab(tableWidget, "表格视图");
    tabWidget->addTab(rawTextView, "原始数据");

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    clearButton = new QPushButton("清空消息", this);
    exportButton = new QPushButton("导出数据", this);

    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(statusLabel);

    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(clearButton, &QPushButton::clicked, this, &MQTTMessageWidget::clearMessages);
    connect(exportButton, &QPushButton::clicked, this, &MQTTMessageWidget::exportMessages);
}

void MQTTMessageWidget::addMessage(const QString &topic, const QByteArray &rawData) const {
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 更完善的二进制数据检测（考虑Unicode）
    bool isBinary = false;
    bool isLikelyText = false;

    // 方法1：尝试解码为UTF-8
    QTextCodec::ConverterState state;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    const QString text = codec->toUnicode(rawData.constData(), rawData.size(), &state);

    // 如果UTF-8解码失败或有大量无效字符，可能是二进制
    if (state.invalidChars > 0 && state.invalidChars * 10 > rawData.size()) {
        isBinary = true;
    } else {
        // 方法2：检查Unicode字符的可打印性
        int nonPrintableCount = 0;
        for (const QChar &c : text) {
            // Unicode字符分类检查
            if (c.category() == QChar::Other_Control || c.category() == QChar::Other_NotAssigned ||
                (c.unicode() >= 0x0080 && c.unicode() <= 0x009F)) { // C1控制字符
                nonPrintableCount++;
            }
        }

        // 如果有大量不可打印Unicode字符，可能是二进制
        if (nonPrintableCount * 10 > text.length()) {
            isBinary = true;
        } else {
            isLikelyText = true;
        }
    }

    // 方法3：检查常见的二进制文件特征
    if (!isBinary && rawData.size() >= 4) {
        // 检查文件魔数（常见的二进制文件开头）
        QByteArray header = rawData.left(4);
        if (header.startsWith("\x89PNG") || header.startsWith("\xFF\xD8\xFF") ||        // PNG, JPEG
            header.startsWith("GIF8") || header.startsWith("%PDF") ||                   // GIF, PDF
            header.startsWith("PK\x03\x04") || header.startsWith("\xD0\xCF\x11\xE0")) { // ZIP, DOC
            isBinary = true;
        }
    }

    QString displayPayload;
    if (isBinary) {
        // 显示二进制信息
        QString hexPreview;
        if (rawData.size() <= 16) {
            // 小二进制数据：显示hex
            hexPreview = rawData.toHex(' ').toUpper();
        } else {
            // 大二进制数据：显示部分hex
            hexPreview = rawData.left(8).toHex(' ').toUpper() + " ... " + rawData.right(8).toHex(' ').toUpper();
        }
        displayPayload = QString("<二进制数据，%1 字节> [%2]").arg(rawData.size()).arg(hexPreview);
    } else if (rawData.size() > 1000 && isLikelyText) {
        // 大文本数据：截断显示
        QString truncatedText = QString::fromUtf8(rawData.constData(), qMin(rawData.size(), 1000));
        displayPayload = truncatedText + QString("... [截断，总长度: %1 字节]").arg(rawData.size());
    } else {
        // 正常文本数据
        displayPayload = QString::fromUtf8(rawData);
    }

    // 更新表格视图
    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);

    tableWidget->setItem(row, 0, new QTableWidgetItem(timestamp));
    tableWidget->setItem(row, 1, new QTableWidgetItem(topic));
    tableWidget->setItem(row, 2, new QTableWidgetItem(displayPayload));
    tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(rawData.size())));

    // 自动滚动到最后一行
    tableWidget->scrollToBottom();

    // 更新原始文本视图
    if (isBinary) {
        // 在原始视图中也显示hex格式
        rawTextView->append(QString("[%1] %2: %3").arg(timestamp, topic, displayPayload));
        rawTextView->append(QString("完整Hex: %1").arg(QString(rawData.toHex(' ').toUpper())));
    } else {
        rawTextView->append(QString("[%1] %2: %3").arg(timestamp, topic, displayPayload));
    }

    rawTextView->verticalScrollBar()->setValue(rawTextView->verticalScrollBar()->maximum());

    // 更新状态
    statusLabel->setText(QString("已接收 %1 条消息").arg(row + 1));
}

void MQTTMessageWidget::clearMessages() {
    tableWidget->setRowCount(0);
    rawTextView->clear();
    statusLabel->setText("消息已清空");
}

void MQTTMessageWidget::exportMessages() {
    QString fileName = QFileDialog::getSaveFileName(this, "导出消息", "mqtt_messages.txt", "文本文件 (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << rawTextView->toPlainText();
            file.close();
            statusLabel->setText("消息已导出到: " + fileName);
        }
    }
}