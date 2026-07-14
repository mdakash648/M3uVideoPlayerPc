#include "XtreamApiClient.h"
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonParseError>

namespace Data {

XtreamApiClient::XtreamApiClient(QObject *parent) 
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {
}

void XtreamApiClient::makeRequest(const Credentials& creds, const QString& action, const QString& extraParams, std::function<void(bool, const QByteArray&)> callback) {
    QString urlStr = QString("%1/player_api.php?username=%2&password=%3").arg(creds.serverUrl, creds.username, creds.password);
    if (!action.isEmpty()) {
        urlStr += "&action=" + action;
    }
    if (!extraParams.isEmpty()) {
        urlStr += "&" + extraParams;
    }

    QUrl url(urlStr);
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            callback(true, reply->readAll());
        } else {
            callback(false, reply->errorString().toUtf8());
        }
        reply->deleteLater();
    });
}

void XtreamApiClient::authenticate(const Credentials& creds, std::function<void(bool, const QString&)> callback) {
    makeRequest(creds, "", "", [callback](bool success, const QByteArray& data) {
        if (!success) {
            callback(false, QString::fromUtf8(data));
            return;
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            callback(false, "Invalid JSON response");
            return;
        }

        QJsonObject root = doc.object();
        if (root.contains("user_info")) {
            QJsonObject userInfo = root["user_info"].toObject();
            if (userInfo.contains("auth") && userInfo["auth"].toInt() == 1) {
                callback(true, "Authentication successful");
            } else {
                callback(false, "Authentication failed");
            }
        } else {
            callback(false, "Invalid user info");
        }
    });
}

void XtreamApiClient::getLiveCategories(const Credentials& creds, std::function<void(bool, const QJsonArray&)> callback) {
    makeRequest(creds, "get_live_categories", "", [callback](bool success, const QByteArray& data) {
        if (!success) {
            callback(false, QJsonArray());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(data);
        callback(true, doc.array());
    });
}

void XtreamApiClient::getLiveStreams(const Credentials& creds, int categoryId, std::function<void(bool, const QJsonArray&)> callback) {
    QString extra = categoryId >= 0 ? QString("category_id=%1").arg(categoryId) : "";
    makeRequest(creds, "get_live_streams", extra, [callback](bool success, const QByteArray& data) {
        if (!success) {
            callback(false, QJsonArray());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(data);
        callback(true, doc.array());
    });
}

void XtreamApiClient::getVodCategories(const Credentials& creds, std::function<void(bool, const QJsonArray&)> callback) {
    makeRequest(creds, "get_vod_categories", "", [callback](bool success, const QByteArray& data) {
        if (!success) {
            callback(false, QJsonArray());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(data);
        callback(true, doc.array());
    });
}

void XtreamApiClient::getVodStreams(const Credentials& creds, int categoryId, std::function<void(bool, const QJsonArray&)> callback) {
    QString extra = categoryId >= 0 ? QString("category_id=%1").arg(categoryId) : "";
    makeRequest(creds, "get_vod_streams", extra, [callback](bool success, const QByteArray& data) {
        if (!success) {
            callback(false, QJsonArray());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(data);
        callback(true, doc.array());
    });
}

void XtreamApiClient::getSeriesCategories(const Credentials& creds, std::function<void(bool, const QJsonArray&)> callback) {
    makeRequest(creds, "get_series_categories", "", [callback](bool success, const QByteArray& data) {
        if (!success) {
            callback(false, QJsonArray());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(data);
        callback(true, doc.array());
    });
}

void XtreamApiClient::getSeries(const Credentials& creds, int categoryId, std::function<void(bool, const QJsonArray&)> callback) {
    QString extra = categoryId >= 0 ? QString("category_id=%1").arg(categoryId) : "";
    makeRequest(creds, "get_series", extra, [callback](bool success, const QByteArray& data) {
        if (!success) {
            callback(false, QJsonArray());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(data);
        callback(true, doc.array());
    });
}

} // namespace Data
