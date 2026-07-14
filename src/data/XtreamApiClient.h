#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Data {

class XtreamApiClient : public QObject {
    Q_OBJECT
public:
    explicit XtreamApiClient(QObject *parent = nullptr);

    struct Credentials {
        QString serverUrl;
        QString username;
        QString password;
    };

    void authenticate(const Credentials& creds, std::function<void(bool success, const QString& message)> callback);
    
    // Live Categories (Groups)
    void getLiveCategories(const Credentials& creds, std::function<void(bool success, const QJsonArray& categories)> callback);
    // Live Streams (Channels)
    void getLiveStreams(const Credentials& creds, int categoryId, std::function<void(bool success, const QJsonArray& streams)> callback);
    
    // VOD Categories
    void getVodCategories(const Credentials& creds, std::function<void(bool success, const QJsonArray& categories)> callback);
    // VOD Streams
    void getVodStreams(const Credentials& creds, int categoryId, std::function<void(bool success, const QJsonArray& streams)> callback);

    // Series Categories
    void getSeriesCategories(const Credentials& creds, std::function<void(bool success, const QJsonArray& categories)> callback);
    // Series Streams
    void getSeries(const Credentials& creds, int categoryId, std::function<void(bool success, const QJsonArray& series)> callback);

private:
    void makeRequest(const Credentials& creds, const QString& action, const QString& extraParams, std::function<void(bool, const QByteArray&)> callback);

    QNetworkAccessManager* m_networkManager;
};

} // namespace Data
