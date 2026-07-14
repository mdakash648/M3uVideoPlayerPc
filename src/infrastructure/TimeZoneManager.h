#pragma once

#include <QObject>
#include <QString>
#include <QTimeZone>
#include "../domain/Enums.h"

namespace Infrastructure {

class TimeZoneManager : public QObject {
    Q_OBJECT
public:
    explicit TimeZoneManager(QObject *parent = nullptr);

    void setMode(Domain::TimeZoneMode mode);
    Domain::TimeZoneMode getMode() const;

    void setManualTimeZone(const QByteArray& ianaId);
    QTimeZone getCurrentTimeZone() const;

private:
    Domain::TimeZoneMode m_mode = Domain::TimeZoneMode::AUTO;
    QTimeZone m_manualTimeZone;
};

} // namespace Infrastructure
