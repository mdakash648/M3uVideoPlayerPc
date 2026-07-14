#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace Domain {

Q_NAMESPACE

enum class ContentType {
    LIVE,
    MOVIE,
    SERIES,
    UNKNOWN
};
Q_ENUM_NS(ContentType)

enum class UpdateFrequency {
    NEVER,
    EVERY_STARTUP,
    HOURLY,
    DAILY,
    WEEKLY
};
Q_ENUM_NS(UpdateFrequency)

enum class TimeZoneMode {
    AUTO,
    MANUAL
};
Q_ENUM_NS(TimeZoneMode)

} // namespace Domain
