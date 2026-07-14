#pragma once

#include <QString>
#include <QDateTime>
#include "Enums.h"

namespace Domain {

struct Playlist {
    int id = 0;
    QString name;
    QString url; // M3U URL, Xtream Codes URL, or local file path
    QString username; // For Xtream Codes
    QString password; // For Xtream Codes
    
    // Config
    UpdateFrequency updateFrequency = UpdateFrequency::NEVER;
    
    // Metadata
    QDateTime lastUpdated;
    QDateTime createdAt;
    
    // Type checking
    bool isXtream() const {
        return !username.isEmpty() && !password.isEmpty();
    }
    bool isLocalFile() const {
        return url.startsWith("file://") || url.startsWith("/");
    }
};

} // namespace Domain
