#include "UpdateChecker.h"
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent) {}

void UpdateChecker::Check(const QUrl &apiUrl, const UpdateCheckRequest &request) {
    // 1. 构造 request
    QNetworkRequest req(apiUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonDocument doc(request.toJson());
    QByteArray body = doc.toJson(QJsonDocument::Compact);

    // 2. 发送网络请求，超时处理
    QNetworkReply *reply = m_nam.post(req, body);
    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->start(4000);
    QObject::connect(timer, &QTimer::timeout, reply, [reply]() { reply->abort(); });

    // 3. 收到回包处理
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const auto err = reply->error();
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        // 3.1 出错处理
        if (err != QNetworkReply::NoError) {
            emit ErrorOccured(QString("网络错误: %1").arg(reply->errorString()));
            return;
        }
        // 3.2 读取回包
        QJsonParseError pe{};
        QJsonDocument doc = QJsonDocument::fromJson(body, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            emit ErrorOccured(QString("返回不是合法 json"));
            return;
        }
        QJsonObject o = doc.object();
        UpdateInfo info;
        info.latest = o.value("version").toString();
        info.downloadUrl = o.value("update_url").toString();
        info.changeLog = o.value("update_log").toString();
        info.updateNeed = o.value("update_need").toBool();
        if (info.updateNeed) {
            emit UpdateAvailable(info);
        } else {
            emit NoUpdate();
        }
    });
}
