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
    NEVER = 0,
    EVERY_STARTUP = 1,
    EVERY_6_HOURS = 2,
    EVERY_12_HOURS = 3,
    EVERY_3_DAYS = 4,
    WEEKLY = 5
};
Q_ENUM_NS(UpdateFrequency)

enum class TimeZoneMode {
    AUTO,
    MANUAL
};
Q_ENUM_NS(TimeZoneMode)

enum class SortMode {
    ASCENDING,
    DESCENDING,
    PLAYLIST_ORDER
};
Q_ENUM_NS(SortMode)

} // namespace Domain
