#pragma once

#include <vector>
#include <optional>
#include "Channel.h"
#include "Group.h"

namespace Domain {

class IChannelRepository {
public:
    virtual ~IChannelRepository() = default;

    // Groups
    virtual bool insertGroup(Group& group) = 0;
    virtual std::vector<Group> getGroupsByPlaylistId(int playlistId) = 0;

    // Channels
    virtual bool insertChannel(Channel& channel) = 0;
    virtual bool insertChannels(std::vector<Channel>& channels) = 0; // Batch insert
    virtual bool updateChannel(const Channel& channel) = 0;
    virtual bool deleteChannel(int id) = 0;
    
    virtual std::optional<Channel> getChannelById(int id) = 0;
    virtual std::vector<Channel> getChannelsByGroupId(int groupId) = 0;
    virtual std::vector<Channel> searchChannels(const QString& query) = 0;
    
    // Favorites
    virtual bool setFavorite(int channelId, bool isFavorite) = 0;
    virtual std::vector<Channel> getFavorites() = 0;
};

} // namespace Domain
