#include "ChannelViewModel.h"
#include <algorithm>

ChannelViewModel::ChannelViewModel(Data::ChannelRepository* channelRepo, QObject *parent)
    : QAbstractListModel(parent), m_channelRepo(channelRepo)
{
}

int ChannelViewModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_channels.size();
}

QVariant ChannelViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_channels.size()) return QVariant();

    const auto& channel = m_channels[index.row()];

    switch (role) {
        case IdRole: return channel.id;
        case NameRole: return channel.name;
        case StreamUrlRole: return channel.streamUrl;
        case LogoUrlRole: return channel.logoUrl;
        case IsFavoriteRole: return channel.isFavorite;
        default: return QVariant();
    }
}

QHash<int, QByteArray> ChannelViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[StreamUrlRole] = "streamUrl";
    roles[LogoUrlRole] = "logoUrl";
    roles[IsFavoriteRole] = "isFavorite";
    return roles;
}

void ChannelViewModel::loadChannels(int groupId) {
    m_favoritesMode = false;
    if (m_channelRepo) {
        auto stdList = m_channelRepo->getChannelsByGroupId(groupId);
        m_sourceChannels.clear();
        for (const auto& c : stdList) {
            m_sourceChannels.append(c);
        }
    }
    applyFilterAndSort();
}

void ChannelViewModel::loadAllChannels(int playlistId) {
    m_favoritesMode = false;
    if (m_channelRepo) {
        auto stdList = m_channelRepo->getChannelsByPlaylistId(playlistId);
        m_sourceChannels.clear();
        for (const auto& c : stdList) {
            m_sourceChannels.append(c);
        }
    }
    applyFilterAndSort();
}

void ChannelViewModel::loadFavorites() {
    m_favoritesMode = true;
    if (m_channelRepo) {
        auto stdList = m_channelRepo->getFavorites();
        m_sourceChannels.clear();
        for (const auto& c : stdList) {
            m_sourceChannels.append(c);
        }
    }
    applyFilterAndSort();
}

void ChannelViewModel::toggleFavorite(int channelId) {
    if (!m_channelRepo) return;

    bool newValue = false;
    bool found = false;
    for (auto& c : m_sourceChannels) {
        if (c.id == channelId) {
            newValue = !c.isFavorite;
            c.isFavorite = newValue;
            found = true;
            break;
        }
    }
    if (!found) return;

    m_channelRepo->setFavorite(channelId, newValue);

    if (m_favoritesMode && !newValue) {
        // Unfavorited while viewing the Favorites folder - it should drop out of this list
        for (int i = 0; i < m_sourceChannels.size(); ++i) {
            if (m_sourceChannels[i].id == channelId) {
                m_sourceChannels.removeAt(i);
                break;
            }
        }
    }

    applyFilterAndSort();
}

void ChannelViewModel::setSearchQuery(const QString& query) {
    if (m_searchQuery == query) return;
    m_searchQuery = query;
    applyFilterAndSort();
}

void ChannelViewModel::setSortMode(int mode) {
    if (m_sortMode == mode) return;
    m_sortMode = mode;
    applyFilterAndSort();
}

void ChannelViewModel::applyFilterAndSort() {
    beginResetModel();

    m_channels.clear();
    if (m_searchQuery.isEmpty()) {
        m_channels = m_sourceChannels;
    } else {
        for (const auto& c : m_sourceChannels) {
            if (c.name.contains(m_searchQuery, Qt::CaseInsensitive)) {
                m_channels.append(c);
            }
        }
    }

    if (m_sortMode == Ascending) {
        std::sort(m_channels.begin(), m_channels.end(), [](const Domain::Channel& a, const Domain::Channel& b) {
            return QString::localeAwareCompare(a.name, b.name) < 0;
        });
    } else if (m_sortMode == Descending) {
        std::sort(m_channels.begin(), m_channels.end(), [](const Domain::Channel& a, const Domain::Channel& b) {
            return QString::localeAwareCompare(a.name, b.name) > 0;
        });
    }
    // PlaylistOrder: keep the order already returned by the repository (ORDER BY orderIndex ASC)

    endResetModel();
}

int ChannelViewModel::findIndexByUrl(const QString& url) const {
    for (int i = 0; i < m_channels.size(); ++i) {
        if (m_channels[i].streamUrl == url) {
            return i;
        }
    }
    return -1;
}

QString ChannelViewModel::channelName(int index) const {
    if (index < 0 || index >= m_channels.size()) return QString();
    return m_channels[index].name;
}

QString ChannelViewModel::channelStreamUrl(int index) const {
    if (index < 0 || index >= m_channels.size()) return QString();
    return m_channels[index].streamUrl;
}
