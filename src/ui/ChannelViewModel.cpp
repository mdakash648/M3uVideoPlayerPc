#include "ChannelViewModel.h"
#include <QDir>
#include <QFileInfo>
#include <QUrl>
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
        case RefererRole: return channel.referer;
        case UserAgentRole: return channel.userAgent;
        case TypeRole: return static_cast<int>(channel.type);
        case PlaylistIdRole: return channel.playlistId;
        case GroupIdRole: return channel.groupId;
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
    roles[RefererRole] = "referer";
    roles[UserAgentRole] = "userAgent";
    roles[TypeRole] = "type";
    roles[PlaylistIdRole] = "playlistId";
    roles[GroupIdRole] = "groupId";
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

const QStringList& ChannelViewModel::videoFileExtensions() {
    static const QStringList exts = {
        "mkv", "mp4", "m4v", "avi", "mov", "wmv", "flv", "webm",
        "ts", "m2ts", "mts", "mpg", "mpeg", "vob", "3gp", "ogv"
    };
    return exts;
}

bool ChannelViewModel::isVideoFile(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return !suffix.isEmpty() && videoFileExtensions().contains(suffix);
}

void ChannelViewModel::loadLocalFolder(const QString& filePath) {
    m_favoritesMode = false;
    m_sourceChannels.clear();

    const QFileInfo openedFile(filePath);
    const QDir dir = openedFile.dir();
    if (dir.exists()) {
        QStringList nameFilters;
        for (const QString& ext : videoFileExtensions()) {
            nameFilters << ("*." + ext);
        }
        // Natural name order so "E01, E02, ... E10" sort like episodes
        const auto entries = dir.entryInfoList(nameFilters, QDir::Files | QDir::Readable,
                                               QDir::Name | QDir::LocaleAware);
        int order = 0;
        for (const QFileInfo& fi : entries) {
            Domain::Channel channel;
            channel.id = 0;          // ephemeral — never stored in the database
            channel.playlistId = 0;  // playlistId 0 disables history/resume saving
            channel.groupId = 0;
            channel.name = fi.completeBaseName();
            channel.streamUrl = fi.absoluteFilePath();
            channel.type = Domain::ContentType::MOVIE; // VOD behavior (seek, auto-next)
            channel.orderIndex = order++;
            m_sourceChannels.append(channel);
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

void ChannelViewModel::deleteChannel(int channelId) {
    if (!m_channelRepo) return;
    if (!m_channelRepo->deleteChannel(channelId)) return;

    for (int i = 0; i < m_sourceChannels.size(); ++i) {
        if (m_sourceChannels[i].id == channelId) {
            m_sourceChannels.removeAt(i);
            break;
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

QString ChannelViewModel::channelReferer(int index) const {
    if (index < 0 || index >= m_channels.size()) return QString();
    return m_channels[index].referer;
}

QString ChannelViewModel::channelUserAgent(int index) const {
    if (index < 0 || index >= m_channels.size()) return QString();
    return m_channels[index].userAgent;
}

int ChannelViewModel::channelType(int index) const {
    if (index < 0 || index >= m_channels.size())
        return static_cast<int>(Domain::ContentType::UNKNOWN);
    return static_cast<int>(m_channels[index].type);
}

int ChannelViewModel::channelPlaylistId(int index) const {
    if (index < 0 || index >= m_channels.size()) return -1;
    return m_channels[index].playlistId;
}
