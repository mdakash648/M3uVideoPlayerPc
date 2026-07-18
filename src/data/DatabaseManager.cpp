#include "DatabaseManager.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

namespace Data {

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::init(const QString &dbName) {
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }

    // Default to app data location if just a filename is provided
    QString dbPath = dbName;
    if (!dbName.contains("/") && !dbName.contains("\\")) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        dbPath = dataDir + "/" + dbName;
    }

    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "Database opened successfully at" << dbPath;
    return createTables();
}

QSqlDatabase DatabaseManager::getDatabase() const {
    return m_db;
}

bool DatabaseManager::createTables() {
    QSqlQuery query(m_db);
    bool success = true;

    // Create Playlists Table
    QString createPlaylists = R"(
        CREATE TABLE IF NOT EXISTS Playlists (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            url TEXT NOT NULL,
            username TEXT,
            password TEXT,
            updateFrequency INTEGER DEFAULT 0,
            lastUpdated TEXT,
            createdAt TEXT
        )
    )";
    if (!query.exec(createPlaylists)) {
        qWarning() << "Failed to create Playlists table:" << query.lastError().text();
        success = false;
    }

    // Create Groups Table
    QString createGroups = R"(
        CREATE TABLE IF NOT EXISTS Groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            playlistId INTEGER NOT NULL,
            name TEXT NOT NULL,
            orderIndex INTEGER DEFAULT 0,
            FOREIGN KEY(playlistId) REFERENCES Playlists(id) ON DELETE CASCADE
        )
    )";
    if (!query.exec(createGroups)) {
        qWarning() << "Failed to create Groups table:" << query.lastError().text();
        success = false;
    }

    // Create Channels Table
    QString createChannels = R"(
        CREATE TABLE IF NOT EXISTS Channels (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            playlistId INTEGER NOT NULL,
            groupId INTEGER NOT NULL,
            name TEXT NOT NULL,
            streamUrl TEXT NOT NULL,
            logoUrl TEXT,
            type INTEGER DEFAULT 3,
            tvgId TEXT,
            tvgName TEXT,
            tvgShift TEXT,
            referer TEXT,
            userAgent TEXT,
            isFavorite INTEGER DEFAULT 0,
            orderIndex INTEGER DEFAULT 0,
            FOREIGN KEY(playlistId) REFERENCES Playlists(id) ON DELETE CASCADE,
            FOREIGN KEY(groupId) REFERENCES Groups(id) ON DELETE CASCADE
        )
    )";
    if (!query.exec(createChannels)) {
        qWarning() << "Failed to create Channels table:" << query.lastError().text();
        success = false;
    }

    // Create History Table
    QString createHistory = R"(
        CREATE TABLE IF NOT EXISTS History (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            channelId INTEGER NOT NULL,
            streamUrl TEXT NOT NULL,
            title TEXT,
            positionMs INTEGER DEFAULT 0,
            durationMs INTEGER DEFAULT 0,
            lastPlayedAt TEXT,
            FOREIGN KEY(channelId) REFERENCES Channels(id) ON DELETE SET NULL
        )
    )";
    if (!query.exec(createHistory)) {
        qWarning() << "Failed to create History table:" << query.lastError().text();
        success = false;
    }

    // Per-movie resume points (Continue / Start Over). Keyed by streamUrl
    // because channel ids are re-created on every playlist refresh.
    QString createMovieResume = R"(
        CREATE TABLE IF NOT EXISTS MovieResume (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            playlistId INTEGER NOT NULL,
            groupId INTEGER,
            groupTitle TEXT,
            streamUrl TEXT NOT NULL UNIQUE,
            title TEXT,
            positionMs INTEGER DEFAULT 0,
            durationMs INTEGER DEFAULT 0,
            lastPlayedAt TEXT
        )
    )";
    if (!query.exec(createMovieResume)) {
        qWarning() << "Failed to create MovieResume table:" << query.lastError().text();
        success = false;
    }

    // Last-played item per playlist for the floating play button.
    // Live TV rows keep positionMs = 0 (channel-only resume, no time skip).
    QString createPlaylistResume = R"(
        CREATE TABLE IF NOT EXISTS PlaylistResume (
            playlistId INTEGER PRIMARY KEY,
            groupId INTEGER,
            groupTitle TEXT,
            streamUrl TEXT NOT NULL,
            title TEXT,
            referer TEXT,
            userAgent TEXT,
            contentType INTEGER DEFAULT 3,
            positionMs INTEGER DEFAULT 0,
            durationMs INTEGER DEFAULT 0,
            lastPlayedAt TEXT
        )
    )";
    if (!query.exec(createPlaylistResume)) {
        qWarning() << "Failed to create PlaylistResume table:" << query.lastError().text();
        success = false;
    }

    // Enable foreign key constraints
    query.exec("PRAGMA foreign_keys = ON;");

    return success;
}

} // namespace Data
