#pragma once

#include <QtSql/QSqlDatabase>
#include <QString>
#include <optional>

namespace Data {

struct MovieResumePoint {
    int playlistId = 0;
    int groupId = 0;
    QString groupTitle;
    QString streamUrl;
    QString title;
    qint64 positionMs = 0;
    qint64 durationMs = 0;
};

struct PlaylistResumePoint {
    int playlistId = 0;
    int groupId = 0;
    QString groupTitle;
    QString streamUrl;
    QString title;
    QString referer;
    QString userAgent;
    int contentType = 3; // Domain::ContentType::UNKNOWN
    qint64 positionMs = 0;
    qint64 durationMs = 0;
};

// Persistence for the two playback-resume systems:
//  - MovieResume: one row per movie/series URL (Continue / Start Over dialog)
//  - PlaylistResume: one row per playlist (floating play button, MX-Player style)
class ResumeRepository {
public:
    explicit ResumeRepository(QSqlDatabase db);

    bool saveMovieResume(const MovieResumePoint& point);
    std::optional<MovieResumePoint> getMovieResume(const QString& streamUrl);
    bool clearMovieResume(const QString& streamUrl);

    bool savePlaylistResume(const PlaylistResumePoint& point);
    std::optional<PlaylistResumePoint> getPlaylistResume(int playlistId);

    bool deleteByPlaylistId(int playlistId);

private:
    QSqlDatabase m_db;
};

} // namespace Data
