#pragma once

#include <QString>
#include "../domain/Enums.h"

namespace Data {

// Reusable Live TV / Movie / Series classification from channel metadata.
// Used by M3uParser on import and by the startup backfill; the result is
// stored in Channels.type so any feature can query it later.
class ContentTypeDetector {
public:
    static Domain::ContentType detect(const QString& name,
                                      const QString& url,
                                      const QString& groupTitle);
};

} // namespace Data
