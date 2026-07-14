#include "ChannelRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDebug>

namespace Data {

ChannelRepository::ChannelRepository(QSqlDatabase db) : m_db(db) {}

// Groups
bool ChannelRepository::insertGroup(Domain::Group& group) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Groups (playlistId, name, orderIndex) VALUES (:playlistId, :name, :orderIndex)");
    query.bindValue(":playlistId", group.playlistId);
    query.bindValue(":name", group.name);
    query.bindValue(":orderIndex", group.orderIndex);

    if (!query.exec()) {
        qWarning() << "Failed to insert group:" << query.lastError().text();
        return false;
    }

    group.id = query.lastInsertId().toInt();
    return true;
}

std::vector<Domain::Group> ChannelRepository::getGroupsByPlaylistId(int playlistId) {
    std::vector<Domain::Group> groups;
    QSqlQuery query(m_db);
    query.prepare("SELECT g.*, (SELECT COUNT(id) FROM Channels c WHERE c.groupId = g.id) as channelCount "
                  "FROM Groups g WHERE g.playlistId = :playlistId ORDER BY g.orderIndex ASC");
    query.bindValue(":playlistId", playlistId);

    if (query.exec()) {
        while (query.next()) {
            Domain::Group group;
            group.id = query.value("id").toInt();
            group.playlistId = query.value("playlistId").toInt();
            group.name = query.value("name").toString();
            group.orderIndex = query.value("orderIndex").toInt();
            group.channelCount = query.value("channelCount").toInt();
            groups.push_back(group);
        }
    }
    return groups;
}

// Channels
bool ChannelRepository::insertChannel(Domain::Channel& channel) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Channels (playlistId, groupId, name, streamUrl, logoUrl, type, tvgId, tvgName, tvgShift, referer, userAgent, isFavorite, orderIndex) "
                  "VALUES (:playlistId, :groupId, :name, :streamUrl, :logoUrl, :type, :tvgId, :tvgName, :tvgShift, :referer, :userAgent, :isFavorite, :orderIndex)");
    
    query.bindValue(":playlistId", channel.playlistId);
    query.bindValue(":groupId", channel.groupId);
    query.bindValue(":name", channel.name);
    query.bindValue(":streamUrl", channel.streamUrl);
    query.bindValue(":logoUrl", channel.logoUrl);
    query.bindValue(":type", static_cast<int>(channel.type));
    query.bindValue(":tvgId", channel.tvgId);
    query.bindValue(":tvgName", channel.tvgName);
    query.bindValue(":tvgShift", channel.tvgShift);
    query.bindValue(":referer", channel.referer);
    query.bindValue(":userAgent", channel.userAgent);
    query.bindValue(":isFavorite", channel.isFavorite ? 1 : 0);
    query.bindValue(":orderIndex", channel.orderIndex);

    if (!query.exec()) {
        qWarning() << "Failed to insert channel:" << query.lastError().text();
        return false;
    }

    channel.id = query.lastInsertId().toInt();
    return true;
}

bool ChannelRepository::insertChannels(std::vector<Domain::Channel>& channels) {
    if (channels.empty()) return true;

    m_db.transaction();
    
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Channels (playlistId, groupId, name, streamUrl, logoUrl, type, tvgId, tvgName, tvgShift, referer, userAgent, isFavorite, orderIndex) "
                  "VALUES (:playlistId, :groupId, :name, :streamUrl, :logoUrl, :type, :tvgId, :tvgName, :tvgShift, :referer, :userAgent, :isFavorite, :orderIndex)");
    
    for (auto& channel : channels) {
        query.bindValue(":playlistId", channel.playlistId);
        query.bindValue(":groupId", channel.groupId);
        query.bindValue(":name", channel.name);
        query.bindValue(":streamUrl", channel.streamUrl);
        query.bindValue(":logoUrl", channel.logoUrl);
        query.bindValue(":type", static_cast<int>(channel.type));
        query.bindValue(":tvgId", channel.tvgId);
        query.bindValue(":tvgName", channel.tvgName);
        query.bindValue(":tvgShift", channel.tvgShift);
        query.bindValue(":referer", channel.referer);
        query.bindValue(":userAgent", channel.userAgent);
        query.bindValue(":isFavorite", channel.isFavorite ? 1 : 0);
        query.bindValue(":orderIndex", channel.orderIndex);

        if (!query.exec()) {
            qWarning() << "Failed to batch insert channel:" << query.lastError().text();
            m_db.rollback();
            return false;
        }
        channel.id = query.lastInsertId().toInt();
    }

    return m_db.commit();
}

bool ChannelRepository::updateChannel(const Domain::Channel& channel) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Channels SET name = :name, streamUrl = :streamUrl, logoUrl = :logoUrl, "
                  "type = :type, tvgId = :tvgId, tvgName = :tvgName, tvgShift = :tvgShift, "
                  "referer = :referer, userAgent = :userAgent, isFavorite = :isFavorite, orderIndex = :orderIndex "
                  "WHERE id = :id");
    
    query.bindValue(":id", channel.id);
    query.bindValue(":name", channel.name);
    query.bindValue(":streamUrl", channel.streamUrl);
    query.bindValue(":logoUrl", channel.logoUrl);
    query.bindValue(":type", static_cast<int>(channel.type));
    query.bindValue(":tvgId", channel.tvgId);
    query.bindValue(":tvgName", channel.tvgName);
    query.bindValue(":tvgShift", channel.tvgShift);
    query.bindValue(":referer", channel.referer);
    query.bindValue(":userAgent", channel.userAgent);
    query.bindValue(":isFavorite", channel.isFavorite ? 1 : 0);
    query.bindValue(":orderIndex", channel.orderIndex);

    if (!query.exec()) {
        qWarning() << "Failed to update channel:" << query.lastError().text();
        return false;
    }
    return true;
}

bool ChannelRepository::deleteChannel(int id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Channels WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

std::optional<Domain::Channel> ChannelRepository::getChannelById(int id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM Channels WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        Domain::Channel channel;
        channel.id = query.value("id").toInt();
        channel.playlistId = query.value("playlistId").toInt();
        channel.groupId = query.value("groupId").toInt();
        channel.name = query.value("name").toString();
        channel.streamUrl = query.value("streamUrl").toString();
        channel.logoUrl = query.value("logoUrl").toString();
        channel.type = static_cast<Domain::ContentType>(query.value("type").toInt());
        channel.tvgId = query.value("tvgId").toString();
        channel.tvgName = query.value("tvgName").toString();
        channel.tvgShift = query.value("tvgShift").toString();
        channel.referer = query.value("referer").toString();
        channel.userAgent = query.value("userAgent").toString();
        channel.isFavorite = query.value("isFavorite").toInt() == 1;
        channel.orderIndex = query.value("orderIndex").toInt();
        return channel;
    }
    return std::nullopt;
}

std::vector<Domain::Channel> ChannelRepository::getChannelsByGroupId(int groupId) {
    std::vector<Domain::Channel> channels;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM Channels WHERE groupId = :groupId ORDER BY orderIndex ASC");
    query.bindValue(":groupId", groupId);

    if (query.exec()) {
        while (query.next()) {
            Domain::Channel channel;
            channel.id = query.value("id").toInt();
            channel.playlistId = query.value("playlistId").toInt();
            channel.groupId = query.value("groupId").toInt();
            channel.name = query.value("name").toString();
            channel.streamUrl = query.value("streamUrl").toString();
            channel.logoUrl = query.value("logoUrl").toString();
            channel.type = static_cast<Domain::ContentType>(query.value("type").toInt());
            channel.tvgId = query.value("tvgId").toString();
            channel.tvgName = query.value("tvgName").toString();
            channel.tvgShift = query.value("tvgShift").toString();
            channel.referer = query.value("referer").toString();
            channel.userAgent = query.value("userAgent").toString();
            channel.isFavorite = query.value("isFavorite").toInt() == 1;
            channel.orderIndex = query.value("orderIndex").toInt();
            channels.push_back(channel);
        }
    }
    return channels;
}

std::vector<Domain::Channel> ChannelRepository::getChannelsByPlaylistId(int playlistId) {
    std::vector<Domain::Channel> channels;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM Channels WHERE playlistId = :playlistId ORDER BY orderIndex ASC");
    query.bindValue(":playlistId", playlistId);

    if (query.exec()) {
        while (query.next()) {
            Domain::Channel channel;
            channel.id = query.value("id").toInt();
            channel.playlistId = query.value("playlistId").toInt();
            channel.groupId = query.value("groupId").toInt();
            channel.name = query.value("name").toString();
            channel.streamUrl = query.value("streamUrl").toString();
            channel.logoUrl = query.value("logoUrl").toString();
            channel.type = static_cast<Domain::ContentType>(query.value("type").toInt());
            channel.tvgId = query.value("tvgId").toString();
            channel.tvgName = query.value("tvgName").toString();
            channel.tvgShift = query.value("tvgShift").toString();
            channel.referer = query.value("referer").toString();
            channel.userAgent = query.value("userAgent").toString();
            channel.isFavorite = query.value("isFavorite").toInt() == 1;
            channel.orderIndex = query.value("orderIndex").toInt();
            channels.push_back(channel);
        }
    }
    return channels;
}

std::vector<Domain::Channel> ChannelRepository::searchChannels(const QString& searchQuery) {
    std::vector<Domain::Channel> channels;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM Channels WHERE name LIKE :query");
    query.bindValue(":query", "%" + searchQuery + "%");

    if (query.exec()) {
        while (query.next()) {
            Domain::Channel channel;
            // Similar parsing...
            channel.id = query.value("id").toInt();
            channel.playlistId = query.value("playlistId").toInt();
            channel.groupId = query.value("groupId").toInt();
            channel.name = query.value("name").toString();
            channel.streamUrl = query.value("streamUrl").toString();
            channel.logoUrl = query.value("logoUrl").toString();
            channel.type = static_cast<Domain::ContentType>(query.value("type").toInt());
            channel.tvgId = query.value("tvgId").toString();
            channel.tvgName = query.value("tvgName").toString();
            channel.tvgShift = query.value("tvgShift").toString();
            channel.referer = query.value("referer").toString();
            channel.userAgent = query.value("userAgent").toString();
            channel.isFavorite = query.value("isFavorite").toInt() == 1;
            channel.orderIndex = query.value("orderIndex").toInt();
            channels.push_back(channel);
        }
    }
    return channels;
}

bool ChannelRepository::setFavorite(int channelId, bool isFavorite) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Channels SET isFavorite = :isFavorite WHERE id = :id");
    query.bindValue(":isFavorite", isFavorite ? 1 : 0);
    query.bindValue(":id", channelId);
    return query.exec();
}

std::vector<Domain::Channel> ChannelRepository::getFavorites() {
    std::vector<Domain::Channel> channels;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM Channels WHERE isFavorite = 1 ORDER BY name ASC");

    if (query.exec()) {
        while (query.next()) {
            Domain::Channel channel;
            // Similar parsing...
            channel.id = query.value("id").toInt();
            channel.playlistId = query.value("playlistId").toInt();
            channel.groupId = query.value("groupId").toInt();
            channel.name = query.value("name").toString();
            channel.streamUrl = query.value("streamUrl").toString();
            channel.logoUrl = query.value("logoUrl").toString();
            channel.type = static_cast<Domain::ContentType>(query.value("type").toInt());
            channel.tvgId = query.value("tvgId").toString();
            channel.tvgName = query.value("tvgName").toString();
            channel.tvgShift = query.value("tvgShift").toString();
            channel.referer = query.value("referer").toString();
            channel.userAgent = query.value("userAgent").toString();
            channel.isFavorite = true;
            channel.orderIndex = query.value("orderIndex").toInt();
            channels.push_back(channel);
        }
    }
    return channels;
}

} // namespace Data
