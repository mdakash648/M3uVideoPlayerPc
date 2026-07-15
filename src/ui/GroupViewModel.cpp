#include "GroupViewModel.h"
#include <algorithm>

GroupViewModel::GroupViewModel(Data::ChannelRepository* channelRepo, QObject *parent)
    : QAbstractListModel(parent), m_channelRepo(channelRepo)
{
}

int GroupViewModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_groups.size();
}

QVariant GroupViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_groups.size()) return QVariant();

    const auto& group = m_groups[index.row()];

    switch (role) {
        case IdRole: return group.id;
        case NameRole: return group.name;
        case PlaylistIdRole: return group.playlistId;
        case OrderIndexRole: return group.orderIndex;
        case ChannelCountRole: return group.channelCount;
        default: return QVariant();
    }
}

QHash<int, QByteArray> GroupViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[PlaylistIdRole] = "playlistId";
    roles[OrderIndexRole] = "orderIndex";
    roles[ChannelCountRole] = "channelCount";
    return roles;
}

void GroupViewModel::loadGroups(int playlistId) {
    if (m_channelRepo) {
        auto stdList = m_channelRepo->getGroupsByPlaylistId(playlistId);
        m_sourceGroups.clear();
        for (const auto& g : stdList) {
            m_sourceGroups.append(g);
        }
        
        int newAllChannelsCount = m_channelRepo->getChannelCountByPlaylistId(playlistId);
        if (m_allChannelsCount != newAllChannelsCount) {
            m_allChannelsCount = newAllChannelsCount;
            emit allChannelsCountChanged();
        }
        
        int newFavoritesCount = m_channelRepo->getFavoritesCount();
        if (m_favoritesCount != newFavoritesCount) {
            m_favoritesCount = newFavoritesCount;
            emit favoritesCountChanged();
        }
    }
    applyFilterAndSort();
}

void GroupViewModel::setSearchQuery(const QString& query) {
    if (m_searchQuery == query) return;
    m_searchQuery = query;
    applyFilterAndSort();
}

void GroupViewModel::setSortMode(int mode) {
    if (m_sortMode == mode) return;
    m_sortMode = mode;
    applyFilterAndSort();
}

void GroupViewModel::applyFilterAndSort() {
    beginResetModel();

    m_groups.clear();

    Domain::Group allChannels;
    allChannels.id = -100;
    allChannels.name = "All Channels";
    allChannels.channelCount = m_allChannelsCount;
    allChannels.orderIndex = -2;
    m_groups.append(allChannels);

    Domain::Group favorites;
    favorites.id = -200;
    favorites.name = "Favorites";
    favorites.channelCount = m_favoritesCount;
    favorites.orderIndex = -1;
    m_groups.append(favorites);

    QList<Domain::Group> filteredGroups;
    if (m_searchQuery.isEmpty()) {
        filteredGroups = m_sourceGroups;
    } else {
        for (const auto& g : m_sourceGroups) {
            if (g.name.contains(m_searchQuery, Qt::CaseInsensitive)) {
                filteredGroups.append(g);
            }
        }
    }

    if (m_sortMode == Ascending) {
        std::sort(filteredGroups.begin(), filteredGroups.end(), [](const Domain::Group& a, const Domain::Group& b) {
            return QString::localeAwareCompare(a.name, b.name) < 0;
        });
    } else if (m_sortMode == Descending) {
        std::sort(filteredGroups.begin(), filteredGroups.end(), [](const Domain::Group& a, const Domain::Group& b) {
            return QString::localeAwareCompare(a.name, b.name) > 0;
        });
    }

    m_groups.append(filteredGroups);

    endResetModel();
}
