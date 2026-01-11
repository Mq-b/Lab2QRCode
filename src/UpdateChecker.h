#pragma once
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>

struct UpdateInfo {
    QString latest;          /**< 最新版本号 */
    QString downloadUrl;     /**< 下载路径 */
    QString changeLog;       /**< 版本更新日志 */
    bool updateNeed = false; /**< 是否需要更新 */
};

struct UpdateCheckRequest {
    QString version; /**< 客户端版本 */
    QString osArch;  /**< 客户端架构 */
    UpdateCheckRequest(const QString _version, const QString _os)
        : version(_version), osArch(_os) {}
    // 序列化成 JSON
    QJsonObject toJson() const {
        QJsonObject o;
        o.insert("version", version);
        o.insert("os-arch", osArch);
        return o;
    }
};
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    void Check(const QUrl &apiUrl, const UpdateCheckRequest &request);
signals:
    void NoUpdate();
    void UpdateAvailable(const UpdateInfo &info);
    void ErrorOccured(const QString &msg);

private:
    QNetworkAccessManager m_nam;
};