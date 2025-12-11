#include "UiConfig.h"
#include "../logging.h"
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QString>
#include <QStringList>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

// 全局的字体，但只在UiConfig.cpp中使用
// 全部通过 Ui::getAppFont() 和 Ui::getAppFont(int) 访问
namespace {
static QFont g_appFont;
}

Ui::Ui()
    : config(loadUiConfig("./setting/config.json")) {}

UiConfig Ui::loadUiConfig(const std::string &filename) {
    UiConfig config;
    config.font_family = "Arial";
    config.bold = false;

    std::ifstream file(filename);
    if (file.is_open()) {
        json ui_json;
        file >> ui_json;

        if (ui_json.contains("ui")) {
            const auto &ui_cfg = ui_json["ui"];
            if (ui_cfg.contains("font_family")) {
                config.font_family = ui_cfg["font_family"].get<std::string>();
            }
            if (ui_cfg.contains("font_file")) {
                config.font_file = ui_cfg["font_file"].get<std::string>();
            }
            if (ui_cfg.contains("bold")) {
                config.bold = ui_cfg["bold"].get<bool>();
            }
        }
    }
    return config;
}

void Ui::applyFont(QApplication &app, const Ui &ui) {
    // 首先为所有可能的字体设置 Emoji fallback
    QStringList emojiSubstitutes;
#ifdef Q_OS_LINUX
    emojiSubstitutes << "Noto Color Emoji" << "Noto Fonts Emoji" << "Noto Emoji";
#elif defined(Q_OS_WIN)
    emojiSubstitutes << "Segoe UI Emoji" << "Segoe UI Symbol";
#elif defined(Q_OS_MACOS)
    emojiSubstitutes << "Apple Color Emoji";
#endif

    QFont font;
    QString fontFamily;

    // 尝试加载自定义字体文件
    std::string fontFile = ui.getFontFile();
    if (!fontFile.empty()) {
        QString qFontFile = QString::fromStdString(fontFile);
        if (QFile::exists(qFontFile)) {
            int fontId = QFontDatabase::addApplicationFont(qFontFile);
            if (fontId != -1) {
                QStringList loadedFamilies = QFontDatabase::applicationFontFamilies(fontId);
                if (!loadedFamilies.isEmpty()) {
                    fontFamily = loadedFamilies.first();
                    spdlog::info("Loaded custom font from file: {} -> {}", fontFile, fontFamily.toStdString());
                }
            } else {
                spdlog::error("Failed to load font file: {}", fontFile);
            }
        } else {
            spdlog::warn("Font file not found: {}", fontFile);
        }
    }

    if (fontFamily.isEmpty()) {
        fontFamily = QString::fromStdString(ui.getFontFamily());
    }

    QStringList fallbackFonts = {fontFamily};

    // fallbacks
#ifdef Q_OS_LINUX
    // Linux 上常用的一些中文字体
    fallbackFonts << "Noto Sans CJK SC" << "Source Han Sans SC"
                  << "WenQuanYi Micro Hei" << "Droid Sans Fallback";
#elif defined(Q_OS_WIN)
    fallbackFonts << "Microsoft YaHei UI" << "Microsoft YaHei" << "Segoe UI";
#elif defined(Q_OS_MACOS)
    fallbackFonts << "PingFang SC" << "Heiti SC";
#endif

    // 通用 fallback
    fallbackFonts << "Arial" << "sans-serif";

    for (const QString &requestedFont : fallbackFonts) {
        if (requestedFont.isEmpty()) {
            continue; // 空字符串跳过,让后续的 fallback fonts 能够匹配
        }

        font.setFamily(requestedFont);
        font.setBold(ui.isBold());

        QFontInfo fontInfo(font);
        QString actualFont = fontInfo.family();

        spdlog::debug("Trying: {} -> Got: {}", requestedFont.toStdString(), actualFont.toStdString());

        QString normalized = requestedFont.simplified().toLower();
        QString actualNormalized = actualFont.simplified().toLower();

        if (actualNormalized.contains(normalized) || normalized.contains(actualNormalized)) {
            spdlog::info(
                "Successfully set font: {} (requested: {})", actualFont.toStdString(), requestedFont.toStdString());

            // 为这个字体设置 Emoji fallback
            for (const QString &emojiFont : emojiSubstitutes) {
                QFont::insertSubstitution(actualFont, emojiFont);
            }

            app.setFont(font);
            g_appFont = font; // Store the application font
            return;
        }
    }

    QFontInfo fontInfo(font);
    spdlog::warn("No suitable font found, using system default: {}", fontInfo.family().toStdString());
    app.setFont(font);
    g_appFont = font; // Store the application font
}

QFont Ui::getAppFont() {
    if (!g_appFont.family().isEmpty()) {
        return g_appFont;
    }
    return QApplication::font();
}

QFont Ui::getAppFont(int pointSize) {
    QFont font;
    if (!g_appFont.family().isEmpty()) {
        font = g_appFont;
    } else {
        font = QApplication::font();
    }
    font.setPointSize(pointSize);
    return font;
}