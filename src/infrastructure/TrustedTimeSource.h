#pragma once

#include <QObject>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <functional>

namespace Infrastructure {

class TrustedTimeSource : public QObject {
    Q_OBJECT
public:
    explicit TrustedTimeSource(QObject *parent = nullptr);

    void fetchNetworkTime(std::function<void(bool success)> callback = nullptr);
    QDateTime getCurrentTime() const;

private:
    QNetworkAccessManager* m_networkManager;
    qint64 m_offsetMs = 0; // offset = networkTime - systemTime
    bool m_isSynced = false;
};

} // namespace Infrastructure
