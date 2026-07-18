#include "ContentTypeDetector.h"
#include "M3uParser.h"
#include <QRegularExpression>
#include <QUrl>

namespace Data {

namespace {

bool containsAnyKeyword(const QString& haystack, const QStringList& keywords) {
    for (const QString& kw : keywords) {
        if (haystack.contains(kw, Qt::CaseInsensitive)) return true;
    }
    return false;
}

bool looksLikeSeriesName(const QString& name) {
    return !M3uParser::detectSeriesName(name).isEmpty();
}

} // namespace

Domain::ContentType ContentTypeDetector::detect(const QString& name,
                                                const QString& url,
                                                const QString& groupTitle) {
    // Strip the |Referer=... header suffix some playlists append to URLs
    QString cleanUrl = url;
    const int pipeIdx = cleanUrl.indexOf('|');
    if (pipeIdx != -1) cleanUrl = cleanUrl.left(pipeIdx);
    const QString path = QUrl(cleanUrl).path().toLower();

    // 1. Xtream-Codes style URL paths are authoritative
    if (path.contains("/movie/"))  return Domain::ContentType::MOVIE;
    if (path.contains("/series/")) return Domain::ContentType::SERIES;
    if (path.contains("/live/"))   return Domain::ContentType::LIVE;

    // 2. VOD file extensions mean a fixed-length video
    static const QStringList vodExtensions = {
        ".mp4", ".mkv", ".avi", ".mov", ".webm", ".m4v", ".flv", ".wmv", ".mpg", ".mpeg"
    };
    for (const QString& ext : vodExtensions) {
        if (path.endsWith(ext)) {
            return looksLikeSeriesName(name) ? Domain::ContentType::SERIES
                                             : Domain::ContentType::MOVIE;
        }
    }

    // 3. Episode markers in the title (S01E02, Season 2, 5x12, EP 07)
    if (looksLikeSeriesName(name)) return Domain::ContentType::SERIES;

    // 4. Group title keywords
    static const QStringList liveKeywords   = { "live", "tv", "channel", "sport", "news", "24/7" };
    static const QStringList movieKeywords  = { "movie", "film", "vod", "cinema" };
    static const QStringList seriesKeywords = { "series", "serial", "show", "drama", "anime" };
    if (containsAnyKeyword(groupTitle, movieKeywords))  return Domain::ContentType::MOVIE;
    if (containsAnyKeyword(groupTitle, seriesKeywords)) return Domain::ContentType::SERIES;
    if (containsAnyKeyword(groupTitle, liveKeywords))   return Domain::ContentType::LIVE;

    // 5. Streaming-only extensions with no VOD/series signals lean live
    if (path.endsWith(".m3u8") || path.endsWith(".ts")) return Domain::ContentType::LIVE;

    // Fallback: LIVE is the safe default — no resume data is stored for it
    return Domain::ContentType::LIVE;
}

} // namespace Data
