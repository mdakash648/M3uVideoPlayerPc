#include "TrustedTimeSource.h"
#include <QNetworkRequest>
#include <QNetworkReply>

namespace Infrastructure {

TrustedTimeSource::TrustedTimeSource(QObject *parent) 
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {
}

void TrustedTimeSource::fetchNetworkTime(std::function<void(bool)> callback) {
    // A simple HTTP HEAD request to a reliable server to get the Date header
    QNetworkRequest request(QUrl("http://worldtimeapi.org/api/timezone/Etc/UTC"));
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Read Date header
            QByteArray dateStr = reply->rawHeader("Date");
            if (!dateStr.isEmpty()) {
                // Parse RFC 2822 date
                QDateTime serverTime = QDateTime::fromString(QString(dateStr), Qt::RFC2822Date);
                if (serverTime.isValid()) {
                    qint64 currentSystemTime = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
                    m_offsetMs = serverTime.toMSecsSinceEpoch() - currentSystemTime;
                    m_isSynced = true;
                    if (callback) callback(true);
                } else {
                    if (callback) callback(false);
                }
            } else {
                if (callback) callback(false);
            }
        } else {
            if (callback) callback(false);
        }
        reply->deleteLater();
    });
}

QDateTime TrustedTimeSource::getCurrentTime() const {
    QDateTime systemTime = QDateTime::currentDateTimeUtc();
    if (m_isSynced) {
        return systemTime.addMSecs(m_offsetMs);
    }
    return systemTime; // Fallback to system time
}

} // namespace Infrastructure
