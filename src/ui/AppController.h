#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <optional>
#include "../infrastructure/TrustedTimeSource.h"
#include "../infrastructure/SettingsManager.h"
#include "../infrastructure/PosterFetchService.h"
#include "../data/DatabaseManager.h"
#include "../data/M3uParser.h"
#include "../data/ResumeRepository.h"
#include "PlaylistViewModel.h"
#include "GroupViewModel.h"
#include "ChannelViewModel.h"

// namespace UI removed for QML registrar compatibility
class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(PlaylistViewModel* playlistViewModel READ playlistViewModel CONSTANT)
    Q_PROPERTY(GroupViewModel* groupViewModel READ groupViewModel CONSTANT)
    Q_PROPERTY(ChannelViewModel* channelViewModel READ channelViewModel CONSTANT)
    Q_PROPERTY(Infrastructure::SettingsManager* settings READ settings CONSTANT)
    Q_PROPERTY(Infrastructure::PosterFetchService* posterFetcher READ posterFetcher CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    static AppController* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    Q_INVOKABLE void init();
    Q_INVOKABLE QString getClipboardText() const;
    Q_INVOKABLE void triggerBackgroundSync();
    Q_INVOKABLE void enterFullscreen();
    Q_INVOKABLE void exitFullscreen(bool wasMaximized, qreal x, qreal y, qreal width, qreal height);
    // Keeps the app window above every other window (VLC's "Always on top").
    Q_INVOKABLE void setAlwaysOnTop(bool enabled);
    // Saves a direct-link stream into the auto-created "History" playlist
    // (group "Movie"). Returns the display name derived from the URL.
    Q_INVOKABLE QString addToHistory(const QString& url, const QString& referer, const QString& userAgent);
    // Plays a direct link. If the URL is an .m3u/.m3u8 playlist of channels,
    // all entries are imported into "History" (keeping their http-referrer /
    // user-agent) and the first one is played; otherwise the URL itself is
    // saved to History and played. Result arrives via directLinkReady().
    Q_INVOKABLE void openDirectLink(const QString& url, const QString& referer, const QString& userAgent);
    // Imports a local .m3u/.m3u8 file into the "History" playlist (used by
    // the file picker and double-clicked files).
    Q_INVOKABLE void openM3uFile(const QString& fileUrlOrPath);
    // Plays a local video file (mkv/mp4/...) WITHOUT saving it to History.
    // Loads every video in the file's folder into channelViewModel so the
    // in-player playlist panel can navigate the folder. Answers via
    // localVideoReady().
    Q_INVOKABLE void openLocalVideo(const QString& fileUrlOrPath);
    // True if the path/URL has a recognized video file extension.
    Q_INVOKABLE bool isLocalVideoFile(const QString& fileUrlOrPath) const;

    // ===== Settings page actions =====
    // Refresh every refreshable playlist now (skips the internal History
    // playlist). Progress arrives via refreshAllProgress/refreshAllFinished.
    Q_INVOKABLE void refreshAllPlaylists();
    // Full app data (playlists + groups + channels + settings) to/from a
    // single JSON file. Accepts file:/// URLs from FileDialog.
    Q_INVOKABLE bool exportBackup(const QString& fileUrl);
    Q_INVOKABLE bool importBackup(const QString& fileUrl);
    // History playlist <-> .m3u file (keeps group-title / headers).
    Q_INVOKABLE bool exportHistoryM3u(const QString& fileUrl);
    Q_INVOKABLE bool importHistoryM3u(const QString& fileUrl);
    // Deletes everything inside the History playlist (groups + channels).
    // The playlist row itself stays — it's recreated on demand anyway.
    Q_INVOKABLE bool clearHistory();
    
    // ===== Demo Data =====
    Q_INVOKABLE bool loadDemoData();
    Q_INVOKABLE bool getAndSetDemoPromptShown();

    // ===== Playback resume =====
    // Movie/series resume point for the Continue / Start Over dialog.
    // Returns {found, positionMs, durationMs}.
    Q_INVOKABLE QVariantMap getMovieResume(const QString& streamUrl);
    Q_INVOKABLE void clearMovieResume(const QString& streamUrl);
    // Called periodically by the player. Upserts MovieResume (movies/series
    // only) and PlaylistResume (always; live TV keeps positionMs = 0).
    Q_INVOKABLE void saveProgress(int playlistId, int groupId, const QString& groupTitle,
                                  const QString& streamUrl, const QString& title,
                                  const QString& referer, const QString& userAgent,
                                  int contentType, qreal positionMs, qreal durationMs);
    // Last-played item of a playlist for the floating play button.
    Q_INVOKABLE QVariantMap getPlaylistResume(int playlistId);
    // Domain::ContentType of a channel as int (LIVE=0/MOVIE=1/SERIES=2/UNKNOWN=3).
    Q_INVOKABLE int channelTypeForUrl(const QString& streamUrl);

    PlaylistViewModel* playlistViewModel() const { return m_playlistViewModel; }
    GroupViewModel* groupViewModel() const { return m_groupViewModel; }
    ChannelViewModel* channelViewModel() const { return m_channelViewModel; }
    Infrastructure::SettingsManager* settings() const { return m_settings; }
    Infrastructure::PosterFetchService* posterFetcher() const { return m_posterFetchService; }

    // Manual "Fetch Posters Now" action for the Settings page — queues a
    // poster-fetch pass for every non-History playlist.
    Q_INVOKABLE void fetchPostersForAllPlaylists();

signals:
    void syncStarted();
    void syncFinished(bool success);
    // Emitted when an .m3u/.m3u8 file is imported (picker or double-click)
    // so the UI can navigate to the Playlists page.
    void m3uFileOpened(const QString& name);
    // Result of openDirectLink(): the (possibly resolved) stream to play.
    void directLinkReady(const QString& streamUrl, const QString& name,
                         const QString& referer, const QString& userAgent);
    void directLinkFailed(const QString& message);
    // Result of openLocalVideo(): a local file ready to play. The folder's
    // videos are already loaded into channelViewModel. Never touches History.
    void localVideoReady(const QString& filePath, const QString& name);
    // Update-all from Settings: emitted per playlist and once at the end.
    void refreshAllProgress(int done, int total, const QString& playlistName);
    void refreshAllFinished(int succeeded, int failed);
    // Backup / restore / history import results for toast messages.
    void backupFinished(bool success, const QString& message);
    void restoreFinished(bool success, const QString& message);
    void historyExportFinished(bool success, const QString& message);
    void historyImportFinished(bool success, const QString& message);
    void historyCleared(bool success, const QString& message);

private:
    void onSyncTimerFired();
    // Ensures the "History" playlist exists; returns its id (or -1 on error).
    int ensureHistoryPlaylist();
    // Re-detects Channels.type for old databases once (flagged in QSettings).
    void backfillContentTypes();
    // Imports parsed M3U entries into History (keeping group titles; entries
    // without a group land in "Movie"). Returns the first imported channel.
    std::optional<Domain::Channel> importIntoHistory(const Data::ParseResult& parsed);

    bool m_startupSyncDone = false; // "on application start" runs only once

    // refreshAllPlaylists() bookkeeping
    int m_refreshAllTotal = 0;
    int m_refreshAllDone = 0;
    int m_refreshAllFailed = 0;

    Data::DatabaseManager* m_dbManager;
    Infrastructure::TrustedTimeSource* m_timeSource;
    Infrastructure::SettingsManager* m_settings;
    Infrastructure::PosterFetchService* m_posterFetchService = nullptr;
    QTimer* m_syncTimer;

    Data::PlaylistRepository* m_playlistRepo = nullptr;
    Data::ChannelRepository* m_channelRepo = nullptr;
    Data::ResumeRepository* m_resumeRepo = nullptr;
    PlaylistViewModel* m_playlistViewModel = nullptr;
    GroupViewModel* m_groupViewModel = nullptr;
    ChannelViewModel* m_channelViewModel = nullptr;
};

