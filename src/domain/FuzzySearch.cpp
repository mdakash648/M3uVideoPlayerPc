#include "FuzzySearch.h"
#include <QRegularExpression>

namespace Domain {

bool FuzzySearch::isMatch(const QString& query, const QString& text) {
    if (query.isEmpty()) return true;
    if (text.isEmpty()) return false;

    QString lowerQuery = query.toLower();
    QString lowerText = text.toLower();

    int queryIdx = 0;
    int textIdx = 0;

    // Subsequence matching (fuzzy search)
    // E.g., query "hbo" matches "Home Box Office"
    while (queryIdx < lowerQuery.length() && textIdx < lowerText.length()) {
        if (lowerQuery[queryIdx] == lowerText[textIdx]) {
            queryIdx++;
        }
        textIdx++;
    }

    return queryIdx == lowerQuery.length();
}

std::vector<Channel> FuzzySearch::search(const QString& query, const std::vector<Channel>& channels) {
    std::vector<Channel> results;
    for (const auto& channel : channels) {
        if (isMatch(query, channel.name)) {
            results.push_back(channel);
        }
    }
    return results;
}

} // namespace Domain
