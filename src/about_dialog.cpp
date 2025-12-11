#include "about_dialog.h"
#include "components/UiConfig.h"
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <spdlog/spdlog.h>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("关于 Lab2QRCode");
    setFixedSize(480, 520);

    initUI();
    loadStyleSheet();
}

void AboutDialog::setVersionInfo(const QString &tag,
                                 const QString &hash,
                                 const QString &branch,
                                 const QString &commitTime,
                                 const QString &buildTime,
                                 const QString &systemVersion,
                                 const QString &kernelVersion,
                                 const QString &architecture) {
    m_tag = tag;
    m_hash = hash;
    m_branch = branch;
    m_commitTime = commitTime;
    m_buildTime = buildTime;
    m_systemVersion = systemVersion;
    m_kernelVersion = kernelVersion;
    m_architecture = architecture;
    // 更新UI显示版本信息
    QWidget *infoWidget = findChild<QWidget *>("infoWidget");
    if (infoWidget) {
        QGridLayout *infoLayout = qobject_cast<QGridLayout *>(infoWidget->layout());
        if (infoLayout) {
            // 清空现有布局内容
            QLayoutItem *child;
            while ((child = infoLayout->takeAt(0)) != nullptr) {
                delete child->widget();
                delete child;
            }

            // 重新添加信息行
            addInfoRow(infoLayout, 0, "📌 版本标签:", m_tag);
            addInfoRow(infoLayout, 1, "🔑 Git Hash:", m_hash.length() > 8 ? m_hash.left(8) + "..." : m_hash);
            addInfoRow(infoLayout, 2, "🌿 代码分支:", m_branch);
            addInfoRow(infoLayout, 3, "⏰ 提交时间:", formatTime(m_commitTime));
            addInfoRow(infoLayout, 4, "🔨 构建时间:", formatTime(m_buildTime));
            addInfoRow(infoLayout, 5, "🖥️ 系统版本:", formatTime(m_systemVersion));
            addInfoRow(infoLayout, 6, "⚙️ 内核版本:", formatTime(m_kernelVersion));
            addInfoRow(infoLayout, 7, "🧩 架构类型:", formatTime(m_architecture));
        }
    }
}

void AboutDialog::initUI() {
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 标题
    m_titleLabel = new QLabel("Lab2QRCode");
    m_titleLabel->setObjectName("title");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel("二维码工具 - 文件转二维码解决方案");
    m_subtitleLabel->setObjectName("subtitle");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_subtitleLabel);

    // 分隔线
    QFrame *separator1 = new QFrame();
    separator1->setObjectName("separator");
    separator1->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(separator1);

    // 版本信息区域
    QWidget *infoWidget = new QWidget();
    infoWidget->setObjectName("infoWidget");
    QGridLayout *infoLayout = new QGridLayout(infoWidget);
    infoLayout->setSpacing(8);
    infoLayout->setContentsMargins(20, 10, 20, 10);
    mainLayout->addWidget(infoWidget);

    // 分隔线
    QFrame *separator2 = new QFrame();
    separator2->setObjectName("separator");
    separator2->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(separator2);

    // 作者信息
    m_authorLabel = new QLabel("👨‍💻 作者: mq白");
    m_authorLabel->setObjectName("author");
    m_authorLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_authorLabel);

    // GitHub链接
    m_githubLabel =
        new QLabel("🌐 GitHub: <a href=\"https://github.com/Mq-b/Lab2QRCode\">https://github.com/Mq-b/Lab2QRCode</a>");
    m_githubLabel->setObjectName("github");
    m_githubLabel->setAlignment(Qt::AlignCenter);
    m_githubLabel->setOpenExternalLinks(true);
    m_githubLabel->setTextFormat(Qt::RichText);
    m_githubLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mainLayout->addWidget(m_githubLabel);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_closeButton = new QPushButton("关闭");
    connect(m_closeButton, &QPushButton::clicked, this, &AboutDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    m_githubButton = new QPushButton("访问 GitHub");
    connect(m_githubButton, &QPushButton::clicked, this, &AboutDialog::onGithubClicked);
    buttonLayout->addWidget(m_githubButton);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
}

void AboutDialog::loadStyleSheet() {
    QFile styleFile("./setting/styles/about_dialog.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        // 动态注入全局字体设置，确保字体生效, 为所有组件设置字体
        QString fontFamily = Ui::getAppFont().family();
        QString fontStyle = QString("QWidget, QLabel, QPushButton { font-family: \"%1\"; }\n").arg(fontFamily);
        setStyleSheet(fontStyle + styleSheet);
    } else {
        spdlog::error("not open file ./setting/styles/about_dialog.qss");
    }
}

void AboutDialog::onGithubClicked() {
    QDesktopServices::openUrl(QUrl("https://github.com/Mq-b/Lab2QRCode"));
}

void AboutDialog::addInfoRow(QGridLayout *layout, int row, const QString &label, const QString &value) {
    QLabel *infoLabel = new QLabel(label);
    infoLabel->setObjectName("infoLabel");

    QLabel *valueLabel = new QLabel(value);
    valueLabel->setObjectName("valueLabel");
    valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(infoLabel, row, 0, Qt::AlignRight);
    layout->addWidget(valueLabel, row, 1, Qt::AlignLeft);
}

QString AboutDialog::formatTime(const QString &timeStr) const {
    if (timeStr.isEmpty()) {
        return "未知";
    }

    // 如果时间字符串包含T（ISO8601格式），进行格式化
    if (timeStr.contains('T')) {
        QDateTime dt = QDateTime::fromString(timeStr, Qt::ISODate);
        if (dt.isValid()) {
            return dt.toString("yyyy-MM-dd hh:mm:ss");
        }
    }

    // 否则原样显示
    return timeStr;
}
