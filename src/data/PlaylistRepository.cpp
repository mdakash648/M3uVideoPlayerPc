#include "PlaylistRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDebug>

namespace Data {

PlaylistRepository::PlaylistRepository(QSqlDatabase db) : m_db(db) {}

bool PlaylistRepository::insertPlaylist(Domain::Playlist& playlist) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Playlists (name, url, username, password, updateFrequency, lastUpdated, createdAt) "
                  "VALUES (:name, :url, :username, :password, :updateFrequency, :lastUpdated, :createdAt)");
    
    query.bindValue(":name", playlist.name);
    query.bindValue(":url", playlist.url);
    query.bindValue(":username", playlist.username);
    query.bindValue(":password", playlist.password);
    query.bindValue(":updateFrequency", static_cast<int>(playlist.updateFrequency));
    
    QDateTime now = QDateTime::currentDateTime();
    playlist.createdAt = now;
    playlist.lastUpdated = now;
    
    query.bindValue(":lastUpdated", now.toString(Qt::ISODate));
    query.bindValue(":createdAt", now.toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to insert playlist:" << query.lastError().text();
        return false;
    }

    playlist.id = query.lastInsertId().toInt();
    return true;
}

bool PlaylistRepository::updatePlaylist(const Domain::Playlist& playlist) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Playlists SET name = :name, url = :url, username = :username, "
                  "password = :password, updateFrequency = :updateFrequency, lastUpdated = :lastUpdated "
                  "WHERE id = :id");
    
    query.bindValue(":id", playlist.id);
    query.bindValue(":name", playlist.name);
    query.bindValue(":url", playlist.url);
    query.bindValue(":username", playlist.username);
    query.bindValue(":password", playlist.password);
    query.bindValue(":updateFrequency", static_cast<int>(playlist.updateFrequency));
    query.bindValue(":lastUpdated", playlist.lastUpdated.toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to update playlist:" << query.lastError().text();
        return false;
    }
    return true;
}

bool PlaylistRepository::deletePlaylist(int id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Playlists WHERE id = :id");
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete playlist:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<Domain::Playlist> PlaylistRepository::getPlaylistById(int id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM Playlists WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        Domain::Playlist playlist;
        playlist.id = query.value("id").toInt();
        playlist.name = query.value("name").toString();
        playlist.url = query.value("url").toString();
        playlist.username = query.value("username").toString();
        playlist.password = query.value("password").toString();
        playlist.updateFrequency = static_cast<Domain::UpdateFrequency>(query.value("updateFrequency").toInt());
        playlist.lastUpdated = QDateTime::fromString(query.value("lastUpdated").toString(), Qt::ISODate);
        playlist.createdAt = QDateTime::fromString(query.value("createdAt").toString(), Qt::ISODate);
        return playlist;
    }
    return std::nullopt;
}

std::vector<Domain::Playlist> PlaylistRepository::getAllPlaylists() {
    std::vector<Domain::Playlist> playlists;
    QSqlQuery query("SELECT * FROM Playlists", m_db);

    while (query.next()) {
        Domain::Playlist playlist;
        playlist.id = query.value("id").toInt();
        playlist.name = query.value("name").toString();
        playlist.url = query.value("url").toString();
        playlist.username = query.value("username").toString();
        playlist.password = query.value("password").toString();
        playlist.updateFrequency = static_cast<Domain::UpdateFrequency>(query.value("updateFrequency").toInt());
        playlist.lastUpdated = QDateTime::fromString(query.value("lastUpdated").toString(), Qt::ISODate);
        playlist.createdAt = QDateTime::fromString(query.value("createdAt").toString(), Qt::ISODate);
        playlists.push_back(playlist);
    }
    return playlists;
}

} // namespace Data
