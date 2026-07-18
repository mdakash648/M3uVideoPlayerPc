#include "ResumeRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDateTime>
#include <QDebug>

namespace Data {

ResumeRepository::ResumeRepository(QSqlDatabase db) : m_db(db) {
}

bool ResumeRepository::saveMovieResume(const MovieResumePoint& point) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO MovieResume (playlistId, groupId, groupTitle, streamUrl, title,
                                 positionMs, durationMs, lastPlayedAt)
        VALUES (:playlistId, :groupId, :groupTitle, :streamUrl, :title,
                :positionMs, :durationMs, :lastPlayedAt)
        ON CONFLICT(streamUrl) DO UPDATE SET
            playlistId = excluded.playlistId,
            groupId = excluded.groupId,
            groupTitle = excluded.groupTitle,
            title = excluded.title,
            positionMs = excluded.positionMs,
            durationMs = excluded.durationMs,
            lastPlayedAt = excluded.lastPlayedAt
    )");
    query.bindValue(":playlistId", point.playlistId);
    query.bindValue(":groupId", point.groupId);
    query.bindValue(":groupTitle", point.groupTitle);
    query.bindValue(":streamUrl", point.streamUrl);
    query.bindValue(":title", point.title);
    query.bindValue(":positionMs", point.positionMs);
    query.bindValue(":durationMs", point.durationMs);
    query.bindValue(":lastPlayedAt", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to save movie resume:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<MovieResumePoint> ResumeRepository::getMovieResume(const QString& streamUrl) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT playlistId, groupId, groupTitle, streamUrl, title, positionMs, durationMs
        FROM MovieResume WHERE streamUrl = :streamUrl
    )");
    query.bindValue(":streamUrl", streamUrl);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    MovieResumePoint point;
    point.playlistId = query.value(0).toInt();
    point.groupId = query.value(1).toInt();
    point.groupTitle = query.value(2).toString();
    point.streamUrl = query.value(3).toString();
    point.title = query.value(4).toString();
    point.positionMs = query.value(5).toLongLong();
    point.durationMs = query.value(6).toLongLong();
    return point;
}

bool ResumeRepository::clearMovieResume(const QString& streamUrl) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM MovieResume WHERE streamUrl = :streamUrl");
    query.bindValue(":streamUrl", streamUrl);
    if (!query.exec()) {
        qWarning() << "Failed to clear movie resume:" << query.lastError().text();
        return false;
    }
    return true;
}

bool ResumeRepository::savePlaylistResume(const PlaylistResumePoint& point) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO PlaylistResume (playlistId, groupId, groupTitle, streamUrl, title,
                                    referer, userAgent, contentType,
                                    positionMs, durationMs, lastPlayedAt)
        VALUES (:playlistId, :groupId, :groupTitle, :streamUrl, :title,
                :referer, :userAgent, :contentType,
                :positionMs, :durationMs, :lastPlayedAt)
        ON CONFLICT(playlistId) DO UPDATE SET
            groupId = excluded.groupId,
            groupTitle = excluded.groupTitle,
            streamUrl = excluded.streamUrl,
            title = excluded.title,
            referer = excluded.referer,
            userAgent = excluded.userAgent,
            contentType = excluded.contentType,
            positionMs = excluded.positionMs,
            durationMs = excluded.durationMs,
            lastPlayedAt = excluded.lastPlayedAt
    )");
    query.bindValue(":playlistId", point.playlistId);
    query.bindValue(":groupId", point.groupId);
    query.bindValue(":groupTitle", point.groupTitle);
    query.bindValue(":streamUrl", point.streamUrl);
    query.bindValue(":title", point.title);
    query.bindValue(":referer", point.referer);
    query.bindValue(":userAgent", point.userAgent);
    query.bindValue(":contentType", point.contentType);
    query.bindValue(":positionMs", point.positionMs);
    query.bindValue(":durationMs", point.durationMs);
    query.bindValue(":lastPlayedAt", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to save playlist resume:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<PlaylistResumePoint> ResumeRepository::getPlaylistResume(int playlistId) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT playlistId, groupId, groupTitle, streamUrl, title, referer, userAgent,
               contentType, positionMs, durationMs
        FROM PlaylistResume WHERE playlistId = :playlistId
    )");
    query.bindValue(":playlistId", playlistId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    PlaylistResumePoint point;
    point.playlistId = query.value(0).toInt();
    point.groupId = query.value(1).toInt();
    point.groupTitle = query.value(2).toString();
    point.streamUrl = query.value(3).toString();
    point.title = query.value(4).toString();
    point.referer = query.value(5).toString();
    point.userAgent = query.value(6).toString();
    point.contentType = query.value(7).toInt();
    point.positionMs = query.value(8).toLongLong();
    point.durationMs = query.value(9).toLongLong();
    return point;
}

bool ResumeRepository::deleteByPlaylistId(int playlistId) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM MovieResume WHERE playlistId = :playlistId");
    query.bindValue(":playlistId", playlistId);
    bool ok = query.exec();

    query.prepare("DELETE FROM PlaylistResume WHERE playlistId = :playlistId");
    query.bindValue(":playlistId", playlistId);
    ok = query.exec() && ok;

    if (!ok) {
        qWarning() << "Failed to delete resume rows for playlist" << playlistId;
    }
    return ok;
}

} // namespace Data
