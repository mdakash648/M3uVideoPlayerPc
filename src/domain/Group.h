#pragma once

#include <QString>

namespace Domain {

struct Group {
    int id = 0;
    int playlistId = 0;
    QString name; // Group name or category
    int orderIndex = 0; // Display order
};

} // namespace Domain
