#pragma once

#include "../domain/IPlaylistRepository.h"
#include <QtSql/QSqlDatabase>

namespace Data {

class PlaylistRepository : public Domain::IPlaylistRepository {
public:
    explicit PlaylistRepository(QSqlDatabase db);
    ~PlaylistRepository() override = default;

    bool insertPlaylist(Domain::Playlist& playlist) override;
    bool updatePlaylist(const Domain::Playlist& playlist) override;
    bool deletePlaylist(int id) override;
    
    std::optional<Domain::Playlist> getPlaylistById(int id) override;
    std::vector<Domain::Playlist> getAllPlaylists() override;

private:
    QSqlDatabase m_db;
};

} // namespace Data
