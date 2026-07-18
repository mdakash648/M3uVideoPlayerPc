#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QFuture>
#include <QFutureWatcher>
#include "../domain/Playlist.h"
#include "../data/PlaylistRepository.h"
#include "../data/ChannelRepository.h"
#include "../infrastructure/SettingsManager.h"
#include "../infrastructure/PosterFetchService.h"

class PlaylistViewModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum PlaylistRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        UrlRole,
        TypeRole,
        LastUpdatedRole,
        UpdateFrequencyRole,
        UsernameRole,
        PasswordRole
    };

    explicit PlaylistViewModel(Data::PlaylistRepository* playlistRepo, Data::ChannelRepository* channelRepo, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadPlaylists();
    Q_INVOKABLE void deletePlaylist(int id);
    Q_INVOKABLE void addPlaylistAsync(const QString& name, const QString& url, bool isXtream, const QString& username, const QString& password,
                                      int updateFrequency = 1 /* Domain::UpdateFrequency::EVERY_STARTUP */);
    // Re-download a playlist's content in place (keeps id/name/settings).
    Q_INVOKABLE void refreshPlaylistAsync(int playlistId);
    // Edit name / source url / credentials / update frequency from Settings.
    // The content itself is untouched — use refreshPlaylistAsync to re-fetch.
    Q_INVOKABLE bool updatePlaylistMeta(int id, const QString& name, const QString& url,
                                        int updateFrequency,
                                        const QString& username, const QString& password);

    // Called once from AppController::init() after both dependencies exist.
    // When set, a successful add/refresh automatically queues a
    // fetchPostersForPlaylist() pass (only if the user has turned the
    // feature on and set an OMDb API key).
    void setPosterFetching(Infrastructure::PosterFetchService* posterFetcher, Infrastructure::SettingsManager* settings);

signals:
    void addPlaylistStarted();
    void addPlaylistFinished(bool success, const QString& message);
    void refreshFinished(int playlistId, bool success, const QString& message);
    // Emitted after a playlist row and its content were removed, so other
    // stores (e.g. resume points) can drop their rows too.
    void playlistDeleted(int playlistId);

private:
    // Shared download+parse+store logic for add and refresh.
    void fetchAndStoreAsync(int playlistId, const QString& url, bool isXtream,
                            const QString& username, const QString& password,
                            bool deleteOnFailure);

    Data::PlaylistRepository* m_playlistRepo;
    Data::ChannelRepository* m_channelRepo;
    Infrastructure::PosterFetchService* m_posterFetcher = nullptr;
    Infrastructure::SettingsManager* m_settings = nullptr;
    QList<Domain::Playlist> m_playlists;
};
