#include "PlaylistViewModel.h"
#include <QtConcurrent>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QFile>
#include <QHash>
#include "../data/M3uParser.h"
#include "../data/XtreamApiClient.h"

namespace {
// Result of the background download/parse step. DB work happens on the main
// thread because the SQLite connection belongs to it.
struct AddPlaylistResult {
    bool success = false;
    QString message;
    Data::ParseResult parseResult;
};
}

PlaylistViewModel::PlaylistViewModel(Data::PlaylistRepository* playlistRepo, Data::ChannelRepository* channelRepo, QObject *parent)
    : QAbstractListModel(parent), m_playlistRepo(playlistRepo), m_channelRepo(channelRepo)
{
    loadPlaylists();
}

int PlaylistViewModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_playlists.size();
}

QVariant PlaylistViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_playlists.size()) return QVariant();

    const auto& playlist = m_playlists[index.row()];

    switch (role) {
        case IdRole: return playlist.id;
        case NameRole: return playlist.name;
        case UrlRole: return playlist.url;
        case TypeRole: return playlist.isXtream();
        case LastUpdatedRole: return QVariant::fromValue(playlist.lastUpdated);
        default: return QVariant();
    }
}

QHash<int, QByteArray> PlaylistViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[UrlRole] = "url";
    roles[TypeRole] = "type";
    roles[LastUpdatedRole] = "lastUpdated";
    return roles;
}

void PlaylistViewModel::loadPlaylists() {
    beginResetModel();
    if (m_playlistRepo) {
        auto stdList = m_playlistRepo->getAllPlaylists();
        m_playlists.clear();
        for (const auto& p : stdList) {
            m_playlists.append(p);
        }
    }
    endResetModel();
}

void PlaylistViewModel::addPlaylistAsync(const QString& name, const QString& url, bool isXtream, const QString& username, const QString& password) {
    emit addPlaylistStarted();

    if (!m_playlistRepo || !m_channelRepo) {
        emit addPlaylistFinished(false, "Repositories not initialized");
        return;
    }
    if (name.trimmed().isEmpty() || url.trimmed().isEmpty()) {
        emit addPlaylistFinished(false, "Name and URL are required");
        return;
    }

    // Insert the playlist row on the main thread — the SQLite connection
    // belongs to this thread and must not be used from a worker thread.
    Domain::Playlist playlist;
    playlist.name = name.trimmed();
    playlist.url = url.trimmed();
    if (isXtream) {
        playlist.username = username;
        playlist.password = password;
    }
    playlist.updateFrequency = Domain::UpdateFrequency::DAILY;

    if (!m_playlistRepo->insertPlaylist(playlist)) {
        emit addPlaylistFinished(false, "Failed to save playlist to database");
        return;
    }

    const int playlistId = playlist.id;
    const QString effectiveUrl = playlist.url;

    QFutureWatcher<AddPlaylistResult>* watcher = new QFutureWatcher<AddPlaylistResult>(this);
    connect(watcher, &QFutureWatcher<AddPlaylistResult>::finished, this, [this, watcher, playlistId]() {
        watcher->deleteLater();
        AddPlaylistResult result = watcher->result();

        if (result.success) {
            // Insert groups first so channels can reference their real DB ids
            QHash<int, int> groupIdMap; // parser temp id -> DB id
            for (auto& g : result.parseResult.groups) {
                const int tempId = g.id;
                if (m_channelRepo->insertGroup(g)) {
                    groupIdMap[tempId] = g.id;
                }
            }
            for (auto& c : result.parseResult.channels) {
                c.groupId = groupIdMap.value(c.groupId, c.groupId);
            }
            if (!m_channelRepo->insertChannels(result.parseResult.channels)) {
                result.success = false;
                result.message = "Failed to save channels to database";
            }
        }

        if (!result.success) {
            // Don't leave an empty playlist behind on failure
            m_playlistRepo->deletePlaylist(playlistId);
        }

        loadPlaylists();
        emit addPlaylistFinished(result.success, result.message);
    });

    // Only download + parse in the background — no DB access here
    QFuture<AddPlaylistResult> future = QtConcurrent::run([playlistId, effectiveUrl, isXtream, username, password]() {
        AddPlaylistResult result;

        QString downloadUrl = effectiveUrl;
        if (isXtream) {
            // Fetch the Xtream server's playlist through its M3U endpoint
            QString base = effectiveUrl;
            while (base.endsWith('/')) base.chop(1);
            downloadUrl = base + "/get.php?username=" + QUrl::toPercentEncoding(username)
                        + "&password=" + QUrl::toPercentEncoding(password)
                        + "&type=m3u_plus&output=ts";
        }

        if (downloadUrl.startsWith("http://", Qt::CaseInsensitive) || downloadUrl.startsWith("https://", Qt::CaseInsensitive)) {
            QNetworkAccessManager manager;
            QNetworkRequest request{QUrl(downloadUrl)};
            QNetworkReply* reply = manager.get(request);
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            if (reply->error() != QNetworkReply::NoError) {
                result.message = "Download failed: " + reply->errorString();
                reply->deleteLater();
                return result;
            }
            const QString content = QString::fromUtf8(reply->readAll());
            reply->deleteLater();

            if (!content.contains("#EXTINF", Qt::CaseInsensitive)) {
                result.message = "Downloaded content is not a valid M3U playlist";
                return result;
            }
            result.parseResult = Data::M3uParser::parse(content, playlistId);
        } else {
            // Local file path (also handle file:// URLs from a file dialog)
            QString localPath = downloadUrl;
            if (localPath.startsWith("file:", Qt::CaseInsensitive)) {
                localPath = QUrl(localPath).toLocalFile();
            }
            if (!QFile::exists(localPath)) {
                result.message = "File not found: " + localPath;
                return result;
            }
            result.parseResult = Data::M3uParser::parseFile(localPath, playlistId);
        }

        if (result.parseResult.channels.empty()) {
            result.message = "No channels found in the playlist";
            return result;
        }

        result.success = true;
        result.message = "Playlist added successfully";
        return result;
    });

    watcher->setFuture(future);
}
