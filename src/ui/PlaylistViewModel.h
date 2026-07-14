#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QFuture>
#include <QFutureWatcher>
#include "../domain/Playlist.h"
#include "../data/PlaylistRepository.h"
#include "../data/ChannelRepository.h"

class PlaylistViewModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum PlaylistRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        UrlRole,
        TypeRole,
        LastUpdatedRole
    };

    explicit PlaylistViewModel(Data::PlaylistRepository* playlistRepo, Data::ChannelRepository* channelRepo, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadPlaylists();
    Q_INVOKABLE void addPlaylistAsync(const QString& name, const QString& url, bool isXtream, const QString& username, const QString& password);

signals:
    void addPlaylistStarted();
    void addPlaylistFinished(bool success, const QString& message);

private:
    Data::PlaylistRepository* m_playlistRepo;
    Data::ChannelRepository* m_channelRepo;
    QList<Domain::Playlist> m_playlists;
};
