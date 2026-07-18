#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include "../domain/Channel.h"
#include "../data/ChannelRepository.h"

class ChannelViewModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum ChannelRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        StreamUrlRole,
        LogoUrlRole,
        IsFavoriteRole,
        RefererRole,
        UserAgentRole,
        TypeRole,
        PlaylistIdRole,
        GroupIdRole
    };

    enum SortMode {
        Ascending = 0,
        Descending = 1,
        PlaylistOrder = 2
    };
    Q_ENUM(SortMode)

    explicit ChannelViewModel(Data::ChannelRepository* channelRepo, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadChannels(int groupId);
    Q_INVOKABLE void loadAllChannels(int playlistId);
    Q_INVOKABLE void loadFavorites();
    // Fills the model with every video file in the given local video's folder
    // (ephemeral — nothing is written to the database). Powers the in-player
    // playlist panel for local playback.
    Q_INVOKABLE void loadLocalFolder(const QString& filePath);

    // Local playback support: recognized video file extensions.
    static const QStringList& videoFileExtensions();
    static bool isVideoFile(const QString& path);
    Q_INVOKABLE void toggleFavorite(int channelId);
    // Permanently removes a channel (used by the History delete buttons).
    Q_INVOKABLE void deleteChannel(int channelId);
    Q_INVOKABLE void setSearchQuery(const QString& query);
    Q_INVOKABLE void setSortMode(int mode);
    Q_INVOKABLE int sortMode() const { return m_sortMode; }

    // Helpers for in-player playlist navigation
    Q_INVOKABLE int findIndexByUrl(const QString& url) const;
    Q_INVOKABLE QString channelName(int index) const;
    Q_INVOKABLE QString channelStreamUrl(int index) const;
    Q_INVOKABLE QString channelReferer(int index) const;
    Q_INVOKABLE QString channelUserAgent(int index) const;
    // Domain::ContentType as int (LIVE=0/MOVIE=1/SERIES=2/UNKNOWN=3)
    Q_INVOKABLE int channelType(int index) const;
    Q_INVOKABLE int channelPlaylistId(int index) const;

private:
    void applyFilterAndSort();

    Data::ChannelRepository* m_channelRepo;
    QList<Domain::Channel> m_sourceChannels; // raw list as loaded from repository
    QList<Domain::Channel> m_channels;       // filtered + sorted list shown to the UI
    QString m_searchQuery;
    int m_sortMode = PlaylistOrder;
    bool m_favoritesMode = false; // true when the currently loaded list is the Favorites folder
};
