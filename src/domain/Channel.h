#pragma once

#include <QString>
#include "Enums.h"

namespace Domain {

struct Channel {
    int id = 0;
    int playlistId = 0;
    int groupId = 0; // Links to Group
    
    QString name;
    QString streamUrl;
    QString logoUrl;
    
    ContentType type = ContentType::UNKNOWN;
    
    // M3U EXTINF attributes
    QString tvgId;
    QString tvgName;
    QString tvgShift;
    
    // HTTP Headers overrides
    QString referer;
    QString userAgent;
    
    // UI Metadata
    bool isFavorite = false;
    int orderIndex = 0; // Sort order in the group
};

} // namespace Domain
