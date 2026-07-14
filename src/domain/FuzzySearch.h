#pragma once

#include <QString>
#include <vector>
#include "Channel.h"

namespace Domain {

class FuzzySearch {
public:
    static bool isMatch(const QString& query, const QString& text);
    static std::vector<Channel> search(const QString& query, const std::vector<Channel>& channels);
};

} // namespace Domain
