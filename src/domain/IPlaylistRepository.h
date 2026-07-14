#pragma once

#include <vector>
#include <optional>
#include "Playlist.h"

namespace Domain {

class IPlaylistRepository {
public:
    virtual ~IPlaylistRepository() = default;

    virtual bool insertPlaylist(Playlist& playlist) = 0;
    virtual bool updatePlaylist(const Playlist& playlist) = 0;
    virtual bool deletePlaylist(int id) = 0;
    
    virtual std::optional<Playlist> getPlaylistById(int id) = 0;
    virtual std::vector<Playlist> getAllPlaylists() = 0;
};

} // namespace Domain
