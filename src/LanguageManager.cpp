#include "LanguageManager.h"
#include <QApplication>
#include <QDebug>
#include <QWidget>
#include <spdlog/spdlog.h>

LanguageManager &LanguageManager::instance() {
    static LanguageManager instance;
    return instance;
}

LanguageManager::LanguageManager(QObject *parent) {
    // 支持的语言映射
    languageNames_.insert("zh_CN", "中文 (简体)");
    languageNames_.insert("en_US", "English");
}

void LanguageManager::init(QString locale) {
    // 如果传入的locale是有效的，则切换语言直接返回
    if (!locale.isEmpty() && languageNames_.contains(locale)) {
        switchLanguage(locale);
        return;
    }
    QString systemLocale = QLocale::system().name();
    QString defaultLocale = languageNames_.contains(systemLocale) ? systemLocale : "en_US";
    switchLanguage(defaultLocale);
}

void LanguageManager::switchLanguage(const QString &locale) {
    if (currentLocale_ == locale) {
        return;
    }
    if (!languageNames_.contains(locale)) {
        spdlog::warn("Unsupported language: {}", locale.toStdString());
        return;
    }
    // 移除旧的翻译器
    qApp->removeTranslator(&translator_);
    // 加载新的翻译器
    if (!translator_.load(QString(":/i18n/app_%1").arg(locale))) {
        spdlog::error("Failed to load translation file: {}", locale.toStdString());
        return;
    }
    // 安装新的翻译器
    qApp->installTranslator(&translator_);
    currentLocale_ = locale;
    spdlog::info("Language switched to: {}", locale.toStdString());
    // 触发Qt全局LanguageChange事件
    QEvent ev(QEvent::LanguageChange);
    qApp->sendEvent(qApp, &ev);
}

QString LanguageManager::currentLocale() const {
    return currentLocale_;
}

QStringList LanguageManager::availableDisPlayNames() const {
    return languageNames_.values();
}
QString LanguageManager::localeFromDisplayName(const QString &displayName) {
    return languageNames_.key(displayName);
}
