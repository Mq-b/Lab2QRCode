#include "message_dialog.h"
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>
MessageDialog::MessageDialog(QWidget *parent)
    : QDialog(parent) {
    setMinimumSize(400, 300);

    // ---------------- 窗口属性 ----------------
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, false);

    // ---------------- 外层主布局 ----------------
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ---------------- 背景容器 ----------------
    QWidget *bg = new QWidget(this);
    bg->setObjectName("bgWidget");
    bg->setStyleSheet(R"(
        #bgWidget {
            background-color: rgb(243,243,243);
            border-radius: 8px;
            border: 1px solid rgb(220,220,220);
        }
    )");

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 60));
    bg->setGraphicsEffect(shadow);

    auto *bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->setSpacing(0);

    // ---------------- Header ----------------
    QWidget *header = new QWidget(bg);
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(15, 15, 15, 15);
    m_titleLabel = new QLabel(header);
    m_titleLabel->setStyleSheet("font-size:16px; font-weight:500;");
    m_titleLabel->setWordWrap(true);
    headerLayout->addWidget(m_titleLabel);

    // ---------------- Content ----------------
    QWidget *content = new QWidget(bg);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(15, 10, 15, 10);
    m_label = new QLabel(content);
    m_label->setWordWrap(true);
    contentLayout->addWidget(m_label);

    // ---------------- Bottom ----------------
    QWidget *bottom = new QWidget(bg);
    bottom->setStyleSheet("background-color: rgb(238,238,238); border-bottom-left-radius:8px; "
                          "border-bottom-right-radius:8px;");
    bottom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->setContentsMargins(15, 10, 15, 10); // 内边距
    bottomLayout->setSpacing(0);

    m_closeBtn = new QPushButton("确定", bottom);
    m_closeBtn->setFixedHeight(28);
    m_closeBtn->setStyleSheet(R"(
        QPushButton {
            border: none;
            border-radius: 4px;
            background-color: rgb(25, 117, 197);
            color: white;
            padding: 6px 12px;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: rgb(0, 130, 220);
        }
        QPushButton:pressed {
            background-color: rgb(0, 90, 170);
        }
    )");

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_closeBtn);
    bottomLayout->addStretch();

    // ---------------- 添加到 bgLayout ----------------
    bgLayout->addWidget(header);
    bgLayout->addWidget(content);
    bgLayout->addWidget(bottom);

    mainLayout->addWidget(bg, 0, Qt::AlignCenter);

    // ---------------- 按钮信号 ----------------
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}
MessageDialog::MessageDialog(QWidget *parent, const QStringList &buttons)
    : QDialog(parent) {
    QString global_font_name = "Microsoft YaHei";

    // ---------------- 窗口属性 ----------------
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, true); // 父窗口透明

    // ---------------- 屏幕尺寸 ----------------
    QScreen *screen = QApplication::primaryScreen();
    QSize screenSize = screen->availableGeometry().size();
    QSize maxDialogSize(screenSize.width() * 2 / 5, screenSize.height() * 3 / 5);

    // ---------------- 外层布局 ----------------
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ---------------- 背景容器 ----------------
    QWidget *bg = new QWidget(this);
    bg->setObjectName("bgWidget");
    bg->setStyleSheet(R"(
        #bgWidget {
            background-color: rgb(243,243,243);
            border-radius: 8px;
            border: 1px solid rgb(220,220,220);
        }
    )");

    auto *shadow = new QGraphicsDropShadowEffect(bg);
    shadow->setBlurRadius(8);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 60));
    bg->setGraphicsEffect(shadow);

    auto *bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(8, 8, 8, 8);
    bgLayout->setSpacing(0);

    QFont globalsettings_font(global_font_name);
    globalsettings_font.setBold(true);
    // ---------------- Header ----------------
    QWidget *header = new QWidget(bg);
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(15, 15, 15, 15);
    m_titleLabel = new QLabel(header);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setFont(globalsettings_font);
    m_titleLabel->setStyleSheet("font-size:16px; font-weight:500;");
    headerLayout->addWidget(m_titleLabel);

    // ---------------- Content（滚动区） ----------------
    QScrollArea *scrollArea = new QScrollArea(bg);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setFrameShadow(QFrame::Plain);
    scrollArea->setStyleSheet("background: transparent;"); // 透明背景
    scrollArea->setWidgetResizable(true);

    QWidget *contentWidget = new QWidget();
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(0);

    m_label = new QLabel(contentWidget);
    m_label->setFont(QFont("Microsoft YaHei"));
    m_label->setWordWrap(true);

    // 根据屏幕 DPI 调整字体大小
    int dpi = screen ? screen->logicalDotsPerInch() : 96;
    int fontSize = 12;
    if (dpi > 120) {
        fontSize = 14;
    }
    if (dpi > 180) {
        fontSize = 16;
    }
    {
        QFont font = m_label->font();
        font.setPointSize(fontSize);
        m_label->setFont(font);
        m_titleLabel->setFont(font);
    }

    contentLayout->addWidget(m_label);
    scrollArea->setWidget(contentWidget);
    scrollArea->setMaximumSize(maxDialogSize); // 限制最大尺寸
    scrollArea->setMinimumSize(300, 200);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // ---------------- Bottom ----------------
    QWidget *bottom = new QWidget(bg);
    bottom->setStyleSheet("background-color: rgb(238,238,238)");
    auto *bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->setContentsMargins(15, 10, 15, 10);
    bottomLayout->setSpacing(10);

    bottomLayout->addStretch(1);
    QFont btnFont;
    btnFont.setBold(true);
    for (const QString &btnText : buttons) {
        QPushButton *btn = new QPushButton(btnText, bottom);
        btn->setFixedHeight(28);
        btn->setFont(btnFont);
        btn->setStyleSheet(R"(
            QPushButton {
                border: none;
                border-radius: 4px;
                background-color: rgb(25, 117, 197);
                color: white;
                padding: 6px 12px;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: rgb(0, 130, 220);
            }
            QPushButton:pressed {
                background-color: rgb(0, 90, 170);
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            m_clickedButton = btn->text();
            accept();
        });

        bottomLayout->addSpacing(10);
        bottomLayout->addWidget(btn);
        bottomLayout->addSpacing(10);
    }
    bottomLayout->addStretch(1);

    // ---------------- 添加控件 ----------------
    bgLayout->addWidget(header);
    bgLayout->addWidget(scrollArea);
    bgLayout->addWidget(bottom);

    mainLayout->addWidget(bg, 0, Qt::AlignCenter);

    adjustSize();

    // ---------------- 居中显示 ----------------
    QRect screenGeom = screen->availableGeometry();
    move(screenGeom.center() - rect().center());
}

// 通用执行函数
int MessageDialog::execWithText(const QString &title, const QString &text, const QString &btnText) {
    m_titleLabel->setText(title);
    m_label->setText(text);
    m_closeBtn->setText(btnText);
    return exec();
}

// ========== 静态接口 ==========
int MessageDialog::information(QWidget *parent, const QString &title, const QString &text) {
    MessageDialog dlg(parent);
    return dlg.execWithText(title, text, "确定");
}

QString
MessageDialog::updateDialog(QWidget *parent, const QString &title, const QString &text, const QStringList &buttons) {
    MessageDialog dlg(parent, buttons);
    dlg.m_titleLabel->setText(title);
    dlg.m_label->setText(text);
    dlg.exec();
    return dlg.m_clickedButton; // 返回点击的按钮文本
}
