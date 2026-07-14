#include "TimeZoneManager.h"

namespace Infrastructure {

TimeZoneManager::TimeZoneManager(QObject *parent) : QObject(parent) {
    m_manualTimeZone = QTimeZone::utc();
}

void TimeZoneManager::setMode(Domain::TimeZoneMode mode) {
    m_mode = mode;
}

Domain::TimeZoneMode TimeZoneManager::getMode() const {
    return m_mode;
}

void TimeZoneManager::setManualTimeZone(const QByteArray& ianaId) {
    QTimeZone tz(ianaId);
    if (tz.isValid()) {
        m_manualTimeZone = tz;
    }
}

QTimeZone TimeZoneManager::getCurrentTimeZone() const {
    if (m_mode == Domain::TimeZoneMode::AUTO) {
        return QTimeZone::systemTimeZone();
    }
    return m_manualTimeZone;
}

} // namespace Infrastructure
