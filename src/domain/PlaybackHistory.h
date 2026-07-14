#pragma once

#include <QString>
#include <QDateTime>

namespace Domain {

struct PlaybackHistory {
    int id = 0;
    int channelId = 0; // Links to Channel
    QString streamUrl; // Store the URL directly in case channel is deleted
    QString title;
    
    // For VOD/Series resume points
    qint64 positionMs = 0; 
    qint64 durationMs = 0;
    
    QDateTime lastPlayedAt;
    
    // If completion is > 95%, we might consider it watched
    bool isCompleted() const {
        if (durationMs <= 0) return false;
        return ((double)positionMs / (double)durationMs) >= 0.95;
    }
};

} // namespace Domain
