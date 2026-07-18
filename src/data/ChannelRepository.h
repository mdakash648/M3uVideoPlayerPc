#pragma once

#include "../domain/IChannelRepository.h"
#include <QtSql/QSqlDatabase>

namespace Data {

class ChannelRepository : public Domain::IChannelRepository {
public:
    explicit ChannelRepository(QSqlDatabase db);
    ~ChannelRepository() override = default;

    // Groups
    bool insertGroup(Domain::Group& group) override;
    std::vector<Domain::Group> getGroupsByPlaylistId(int playlistId) override;

    // Channels
    bool insertChannel(Domain::Channel& channel) override;
    bool insertChannels(std::vector<Domain::Channel>& channels) override; // Batch insert
    bool updateChannel(const Domain::Channel& channel) override;
    bool deleteChannel(int id) override;
    
    std::optional<Domain::Channel> getChannelById(int id) override;
    std::vector<Domain::Channel> getChannelsByGroupId(int groupId) override;
    std::vector<Domain::Channel> getChannelsByPlaylistId(int playlistId) override;
    int getChannelCountByPlaylistId(int playlistId) override;
    std::vector<Domain::Channel> searchChannels(const QString& query) override;
    bool deleteContentByPlaylistId(int playlistId) override;
    
    // Favorites
    bool setFavorite(int channelId, bool isFavorite) override;
    std::vector<Domain::Channel> getFavorites() override;
    int getFavoritesCount() override;

private:
    QSqlDatabase m_db;
};

} // namespace Data
