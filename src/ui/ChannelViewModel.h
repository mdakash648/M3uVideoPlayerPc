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
        UserAgentRole
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
    Q_INVOKABLE void toggleFavorite(int channelId);
    Q_INVOKABLE void setSearchQuery(const QString& query);
    Q_INVOKABLE void setSortMode(int mode);
    Q_INVOKABLE int sortMode() const { return m_sortMode; }

    // Helpers for in-player playlist navigation
    Q_INVOKABLE int findIndexByUrl(const QString& url) const;
    Q_INVOKABLE QString channelName(int index) const;
    Q_INVOKABLE QString channelStreamUrl(int index) const;
    Q_INVOKABLE QString channelReferer(int index) const;
    Q_INVOKABLE QString channelUserAgent(int index) const;

private:
    void applyFilterAndSort();

    Data::ChannelRepository* m_channelRepo;
    QList<Domain::Channel> m_sourceChannels; // raw list as loaded from repository
    QList<Domain::Channel> m_channels;       // filtered + sorted list shown to the UI
    QString m_searchQuery;
    int m_sortMode = PlaylistOrder;
    bool m_favoritesMode = false; // true when the currently loaded list is the Favorites folder
};
