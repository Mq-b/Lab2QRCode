#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QMap>
#include <QObject>
#include <QTranslator>

// 多语言管理器单例类
class LanguageManager : public QObject {
    Q_OBJECT
public:
    // 获取单例
    static LanguageManager &instance();
    // 禁用拷贝和移动
    LanguageManager(const LanguageManager &) = delete;
    LanguageManager &operator=(const LanguageManager &) = delete;
    // 初始化语言函数，应当在BarcodeWidget的构造函数调用
    // 参数传入zh_CN或en_US此类字符串
    // 从setting读取config.json传入最后一次选择的locale，若是默认参数则自动判断
    void init(QString locale = {});
    // 切换语言
    // 参数传入zh_CN或en_US此类字符串
    void switchLanguage(const QString &locale);
    // 返回当前locale,eg:zh_CN
    QString currentLocale() const;
    // 返回所有可用语言的名称，如中文 (简体),English
    QStringList availableDisPlayNames() const;
    // 根据displayName获取locale,在触发切换语言Action时获取locale使用
    QString localeFromDisplayName(const QString &displayName);

private:
    explicit LanguageManager(QObject *parent = nullptr);

private:
    QTranslator translator_;
    QString currentLocale_;
    QMap<QString, QString> languageNames_;
};

#endif // LANGUAGEMANAGER_H