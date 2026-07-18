#include "AppController.h"
#include <QDebug>
#include <QGuiApplication>
#include <QWindow>
#include <QClipboard>
#include <QScreen>
#include <QUrl>
#include <QFileInfo>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSaveFile>
#include <limits>
#include "../infrastructure/TrustedTimeSource.h"
#include "PlaylistViewModel.h"
#include "GroupViewModel.h"
#include "ChannelViewModel.h"
#include "../data/DatabaseManager.h"
#include "../data/PlaylistRepository.h"
#include "../data/ChannelRepository.h"
#include "../data/ResumeRepository.h"
#include "../data/ContentTypeDetector.h"
#include <QSettings>

static AppController *s_instance = nullptr;

AppController::AppController(QObject *parent)
    : QObject(parent),
      m_dbManager(new Data::DatabaseManager(this)),
      m_timeSource(new Infrastructure::TrustedTimeSource(this)),
      m_settings(new Infrastructure::SettingsManager(this)),
      m_syncTimer(new QTimer(this))
{
    s_instance = this;

    // Setup background sync timer (e.g. check every hour)
    connect(m_syncTimer, &QTimer::timeout, this, &AppController::onSyncTimerFired);
    m_syncTimer->setInterval(60 * 60 * 1000); // 1 hour
}

AppController::~AppController() {
    if (m_posterFetchService) delete m_posterFetchService;
    if (m_channelViewModel) delete m_channelViewModel;
    if (m_groupViewModel) delete m_groupViewModel;
    if (m_playlistViewModel) delete m_playlistViewModel;
    if (m_resumeRepo) delete m_resumeRepo;
    if (m_channelRepo) delete m_channelRepo;
    if (m_playlistRepo) delete m_playlistRepo;

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

AppController* AppController::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    if (!s_instance) {
        return new AppController();
    }
    return s_instance; // Although QML_SINGLETON implies engine manages it, we keep it safe
}

void AppController::init() {
    qDebug() << "AppController initializing...";

    // Initialize Database
    if (m_dbManager->init("m3uplayer.db")) {
        qDebug() << "Database initialized successfully.";

        m_playlistRepo = new Data::PlaylistRepository(m_dbManager->getDatabase());
        m_channelRepo = new Data::ChannelRepository(m_dbManager->getDatabase());
        m_resumeRepo = new Data::ResumeRepository(m_dbManager->getDatabase());
        m_playlistViewModel = new PlaylistViewModel(m_playlistRepo, m_channelRepo, this);
        m_groupViewModel = new GroupViewModel(m_channelRepo, this);
        m_channelViewModel = new ChannelViewModel(m_channelRepo, this);

        // Auto Movie Poster (OMDb): fetches missing tvg-logo/poster images
        // for movie/series entries. On by default; only actually runs once
        // the user pastes an OMDb API key into Settings.
        m_posterFetchService = new Infrastructure::PosterFetchService(m_settings, m_channelRepo, this);
        m_playlistViewModel->setPosterFetching(m_posterFetchService, m_settings);

        // Old databases have everything typed LIVE — re-detect once so the
        // resume logic can tell movies/series from live TV.
        backfillContentTypes();

        // Resume rows must not outlive their playlist.
        connect(m_playlistViewModel, &PlaylistViewModel::playlistDeleted, this,
                [this](int playlistId) {
            if (m_resumeRepo) m_resumeRepo->deleteByPlaylistId(playlistId);
        });

        // Count results of refreshAllPlaylists(). Single permanent connection
        // — only active while a refresh-all round is in flight.
        connect(m_playlistViewModel, &PlaylistViewModel::refreshFinished, this,
                [this](int playlistId, bool success, const QString&) {
            Q_UNUSED(playlistId);
            if (m_refreshAllTotal <= 0) return;
            ++m_refreshAllDone;
            if (!success) ++m_refreshAllFailed;
            emit refreshAllProgress(m_refreshAllDone, m_refreshAllTotal, QString());
            if (m_refreshAllDone >= m_refreshAllTotal) {
                emit refreshAllFinished(m_refreshAllTotal - m_refreshAllFailed, m_refreshAllFailed);
                m_refreshAllTotal = 0;
            }
        });
    } else {
        qWarning() << "Database initialization failed!";
    }
    
    // Sync app time against a network time source first: playlist update
    // scheduling must not trust a possibly-wrong PC clock. The initial sync
    // runs once the fetch answers (or immediately on failure, falling back
    // to system time).
    m_timeSource->fetchNetworkTime([this](bool success) {
        qDebug() << "Network time sync" << (success ? "succeeded" : "failed — using system time");
        triggerBackgroundSync();
    });

    // Start periodic timer
    m_syncTimer->start();
}

QString AppController::getClipboardText() const {
    if (auto clip = QGuiApplication::clipboard()) {
        return clip->text();
    }
    return QString();
}

void AppController::triggerBackgroundSync() {
    onSyncTimerFired();
}

void AppController::enterFullscreen() {
    QWindow *win = QGuiApplication::focusWindow();
    if (!win) {
        return;
    }
    QScreen *screen = win->screen();
    if (!screen) {
        return;
    }

    // "Fake" fullscreen: strip the window frame and resize to the full
    // screen geometry. We deliberately avoid Qt::WindowFullScreen here —
    // on Windows that state change goes through the OS's exclusive
    // fullscreen enter/exit animation, which is what caused the visible
    // minimize/maximize flicker. A plain frameless + geometry change is a
    // single, instant, unanimated operation — this is the same technique
    // players like VLC/MPC-HC use for a snap-instant fullscreen toggle.
    win->setFlag(Qt::FramelessWindowHint, true);
    win->setWindowState(Qt::WindowNoState);
    win->setGeometry(screen->geometry());
}

void AppController::exitFullscreen(bool wasMaximized, qreal x, qreal y, qreal width, qreal height) {
    QWindow *win = QGuiApplication::focusWindow();
    if (!win) {
        return;
    }

    win->setFlag(Qt::FramelessWindowHint, false);

    if (wasMaximized) {
        // Never having entered a real fullscreen WindowState means this is a
        // normal, non-animated maximize — no flicker, unlike restoring from
        // an actual Qt::WindowFullScreen state.
        win->setWindowState(Qt::WindowMaximized);
    } else if (width > 0 && height > 0) {
        win->setWindowState(Qt::WindowNoState);
        win->setGeometry(int(x), int(y), int(width), int(height));
    }
}

int AppController::ensureHistoryPlaylist() {
    if (!m_playlistRepo) return -1;

    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        if (p.name == QStringLiteral("History")) {
            return p.id;
        }
    }

    Domain::Playlist history;
    history.name = QStringLiteral("History");
    history.url = QStringLiteral("internal://history"); // not refreshable
    history.updateFrequency = Domain::UpdateFrequency::NEVER;
    if (!m_playlistRepo->insertPlaylist(history)) {
        qWarning() << "Failed to create History playlist";
        return -1;
    }
    m_playlistViewModel->loadPlaylists();
    return history.id;
}

std::optional<Domain::Channel> AppController::importIntoHistory(const Data::ParseResult& parsed) {
    const int historyId = ensureHistoryPlaylist();
    if (historyId < 0 || !m_channelRepo) return std::nullopt;

    // Existing state of History for dedup + group reuse
    const auto existingChannels = m_channelRepo->getChannelsByPlaylistId(historyId);
    auto existingGroups = m_channelRepo->getGroupsByPlaylistId(historyId);

    // parser temp group id -> group name
    QHash<int, QString> tempGroupNames;
    for (const auto& g : parsed.groups) {
        tempGroupNames[g.id] = g.name;
    }

    auto groupIdFor = [&](const QString& name) -> int {
        for (const auto& g : existingGroups) {
            if (g.name == name) return g.id;
        }
        Domain::Group group;
        group.playlistId = historyId;
        group.name = name;
        group.orderIndex = static_cast<int>(existingGroups.size());
        if (!m_channelRepo->insertGroup(group)) return -1;
        existingGroups.push_back(group);
        return group.id;
    };

    std::optional<Domain::Channel> first;
    int orderIndex = static_cast<int>(existingChannels.size());

    for (const auto& parsedChannel : parsed.channels) {
        // The parser defaults empty group-title to "Movie" already; map the
        // temp group id to a real History group.
        const QString groupName = tempGroupNames.value(parsedChannel.groupId,
                                                       QStringLiteral("Movie"));

        // Dedup by stream URL: refresh headers/name on the existing row
        bool duplicate = false;
        for (const auto& c : existingChannels) {
            if (c.streamUrl == parsedChannel.streamUrl) {
                Domain::Channel updated = c;
                updated.name = parsedChannel.name;
                updated.referer = parsedChannel.referer;
                updated.userAgent = parsedChannel.userAgent;
                m_channelRepo->updateChannel(updated);
                if (!first) first = updated;
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        const int groupId = groupIdFor(groupName);
        if (groupId < 0) continue;

        Domain::Channel channel = parsedChannel;
        channel.id = 0;
        channel.playlistId = historyId;
        channel.groupId = groupId;
        if (channel.type == Domain::ContentType::UNKNOWN) {
            channel.type = Domain::ContentType::MOVIE;
        }
        channel.orderIndex = orderIndex++;
        if (m_channelRepo->insertChannel(channel) && !first) {
            first = channel;
        }
    }

    if (m_posterFetchService && m_settings && m_settings->autoPosterFetchEnabled()
        && !m_settings->omdbApiKey().trimmed().isEmpty()) {
        m_posterFetchService->fetchPostersForPlaylist(historyId, /*onlyMissing=*/true);
    }

    return first;
}

QString AppController::addToHistory(const QString& url, const QString& referer, const QString& userAgent) {
    const QString trimmedUrl = url.trimmed();
    if (trimmedUrl.isEmpty()) return QString();

    // Derive a human-readable name from the URL's file name
    QString name = QUrl(trimmedUrl).fileName();
    if (name.isEmpty()) name = trimmedUrl;

    // Series episodes ("Loki S01E02") get a group named after the series;
    // plain videos go to "Movie".
    const QString series = Data::M3uParser::detectSeriesName(name);
    const QString groupName = series.isEmpty() ? QStringLiteral("Movie") : series;

    Data::ParseResult single;
    Domain::Group group;
    group.id = 1; // temp id
    group.name = groupName;
    single.groups.push_back(group);

    Domain::Channel channel;
    channel.groupId = 1;
    channel.name = name;
    channel.streamUrl = trimmedUrl;
    channel.type = series.isEmpty() ? Domain::ContentType::MOVIE
                                    : Domain::ContentType::SERIES;
    channel.referer = referer.trimmed();
    channel.userAgent = userAgent.trimmed();
    single.channels.push_back(channel);

    importIntoHistory(single);
    return name;
}

void AppController::openDirectLink(const QString& url, const QString& referer, const QString& userAgent) {
    const QString trimmedUrl = url.trimmed();
    if (trimmedUrl.isEmpty()) {
        emit directLinkFailed(QStringLiteral("Empty URL"));
        return;
    }

    // Plain video URLs (.mp4/.ts/...) go straight to History and play.
    // .m3u/.m3u8 could be either a playlist of channels or an HLS stream —
    // download it in the background and look at the content to decide.
    const QString path = QUrl(trimmedUrl).path();
    const bool maybePlaylist = path.endsWith(".m3u", Qt::CaseInsensitive)
                            || path.endsWith(".m3u8", Qt::CaseInsensitive);
    if (!maybePlaylist) {
        const QString name = addToHistory(trimmedUrl, referer, userAgent);
        emit directLinkReady(trimmedUrl, name, referer.trimmed(), userAgent.trimmed());
        return;
    }

    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, trimmedUrl, referer, userAgent]() {
        watcher->deleteLater();
        const QString content = watcher->result();

        // #EXTINF entries with their own URLs = a channel playlist. An HLS
        // media playlist instead lists segments under #EXT-X-* tags.
        const bool isChannelPlaylist = content.contains("#EXTINF", Qt::CaseInsensitive)
                                    && !content.contains("#EXT-X-TARGETDURATION", Qt::CaseInsensitive)
                                    && !content.contains("#EXT-X-STREAM-INF", Qt::CaseInsensitive);

        if (isChannelPlaylist) {
            // Parse and import every entry — including its http-referrer /
            // user-agent attributes — then play the first one.
            Data::ParseResult parsed = Data::M3uParser::parse(content, /*playlistId=*/0);
            const auto first = importIntoHistory(parsed);
            if (first) {
                emit directLinkReady(first->streamUrl, first->name,
                                     first->referer, first->userAgent);
            } else {
                emit directLinkFailed(QStringLiteral("No channels found in the playlist"));
            }
        } else {
            // HLS stream (or download failed): play the URL directly with
            // whatever headers the user typed.
            const QString name = addToHistory(trimmedUrl, referer, userAgent);
            emit directLinkReady(trimmedUrl, name, referer.trimmed(), userAgent.trimmed());
        }
    });

    watcher->setFuture(QtConcurrent::run([trimmedUrl, referer, userAgent]() -> QString {
        if (!trimmedUrl.startsWith("http://", Qt::CaseInsensitive)
            && !trimmedUrl.startsWith("https://", Qt::CaseInsensitive)) {
            return QString();
        }
        QNetworkAccessManager manager;
        QNetworkRequest request{QUrl(trimmedUrl)};
        if (!referer.trimmed().isEmpty())
            request.setRawHeader("Referer", referer.trimmed().toUtf8());
        if (!userAgent.trimmed().isEmpty())
            request.setRawHeader("User-Agent", userAgent.trimmed().toUtf8());
        QNetworkReply* reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        QString content;
        if (reply->error() == QNetworkReply::NoError) {
            content = QString::fromUtf8(reply->readAll());
        }
        reply->deleteLater();
        return content;
    }));
}

void AppController::openM3uFile(const QString& fileUrlOrPath) {
    // Accept both file:/// URLs (from FileDialog / QML) and plain paths
    // (from a double-clicked file passed on the command line)
    QString localPath = fileUrlOrPath;
    if (localPath.startsWith("file:", Qt::CaseInsensitive)) {
        localPath = QUrl(localPath).toLocalFile();
    }

    QFileInfo info(localPath);
    if (!info.exists()) {
        qWarning() << "M3U file not found:" << localPath;
        return;
    }

    // Import the file's entries into the shared "History" playlist —
    // no separate playlist per file.
    Data::ParseResult parsed = Data::M3uParser::parseFile(info.absoluteFilePath(), /*playlistId=*/0);
    if (parsed.channels.empty()) {
        qWarning() << "No channels found in" << localPath;
        return;
    }
    const auto first = importIntoHistory(parsed);
    if (first) {
        emit directLinkReady(first->streamUrl, first->name, first->referer, first->userAgent);
    } else {
        emit m3uFileOpened(QStringLiteral("History"));
    }
}

void AppController::onSyncTimerFired() {
    if (!m_playlistRepo || !m_playlistViewModel) return;

    qDebug() << "Checking playlists for scheduled updates...";
    emit syncStarted();

    // Trusted (network-synced) time in the user's selected zone — never the
    // raw PC clock, so a wrong system date can't break the schedule.
    const QDateTime now = m_timeSource->getCurrentTime()
                              .toTimeZone(m_settings->effectiveTimeZone());
    const auto playlists = m_playlistRepo->getAllPlaylists();
    bool anyRefreshed = false;

    for (const auto& p : playlists) {
        bool due = false;
        const qint64 age = p.lastUpdated.isValid() ? p.lastUpdated.secsTo(now)
                                                   : std::numeric_limits<qint64>::max();
        switch (p.updateFrequency) {
            case Domain::UpdateFrequency::EVERY_STARTUP:
                due = !m_startupSyncDone;
                break;
            case Domain::UpdateFrequency::EVERY_6_HOURS:  due = age >= 6 * 3600; break;
            case Domain::UpdateFrequency::EVERY_12_HOURS: due = age >= 12 * 3600; break;
            case Domain::UpdateFrequency::EVERY_3_DAYS:   due = age >= 3 * 86400; break;
            case Domain::UpdateFrequency::WEEKLY:         due = age >= 7 * 86400; break;
            case Domain::UpdateFrequency::NEVER:
            default:
                due = false;
                break;
        }

        if (due) {
            qDebug() << "Auto-updating playlist" << p.id << p.name;
            m_playlistViewModel->refreshPlaylistAsync(p.id);
            anyRefreshed = true;
        }
    }

    m_startupSyncDone = true;
    emit syncFinished(true);
    Q_UNUSED(anyRefreshed);
}

// ===== Settings page actions =====

namespace {
// FileDialog hands us file:/// URLs; command line / tests use plain paths.
QString toLocalPath(const QString& fileUrlOrPath) {
    if (fileUrlOrPath.startsWith("file:", Qt::CaseInsensitive)) {
        return QUrl(fileUrlOrPath).toLocalFile();
    }
    return fileUrlOrPath;
}
}

void AppController::fetchPostersForAllPlaylists() {
    if (!m_posterFetchService || !m_playlistRepo) return;
    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        m_posterFetchService->fetchPostersForPlaylist(p.id, /*onlyMissing=*/true);
    }
}

void AppController::refreshAllPlaylists() {
    if (!m_playlistRepo || !m_playlistViewModel) return;
    if (m_refreshAllTotal > 0) return; // already running

    // History is internal (url "internal://history") — nothing to re-download.
    std::vector<Domain::Playlist> refreshable;
    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        if (!p.url.startsWith("internal://")) {
            refreshable.push_back(p);
        }
    }

    if (refreshable.empty()) {
        emit refreshAllFinished(0, 0);
        return;
    }

    m_refreshAllTotal = static_cast<int>(refreshable.size());
    m_refreshAllDone = 0;
    m_refreshAllFailed = 0;
    emit refreshAllProgress(0, m_refreshAllTotal, QString());

    for (const auto& p : refreshable) {
        m_playlistViewModel->refreshPlaylistAsync(p.id);
    }
}

bool AppController::exportBackup(const QString& fileUrl) {
    if (!m_playlistRepo || !m_channelRepo) {
        emit backupFinished(false, QStringLiteral("App not initialized"));
        return false;
    }

    QJsonArray playlistsArr;
    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        QJsonObject po;
        po["name"] = p.name;
        po["url"] = p.url;
        po["username"] = p.username;
        po["password"] = p.password;
        po["updateFrequency"] = static_cast<int>(p.updateFrequency);
        po["lastUpdated"] = p.lastUpdated.toString(Qt::ISODate);
        po["createdAt"] = p.createdAt.toString(Qt::ISODate);

        QJsonArray groupsArr;
        for (const auto& g : m_channelRepo->getGroupsByPlaylistId(p.id)) {
            QJsonObject go;
            go["name"] = g.name;
            go["orderIndex"] = g.orderIndex;

            QJsonArray channelsArr;
            for (const auto& c : m_channelRepo->getChannelsByGroupId(g.id)) {
                QJsonObject co;
                co["name"] = c.name;
                co["streamUrl"] = c.streamUrl;
                co["logoUrl"] = c.logoUrl;
                co["type"] = static_cast<int>(c.type);
                co["tvgId"] = c.tvgId;
                co["tvgName"] = c.tvgName;
                co["tvgShift"] = c.tvgShift;
                co["referer"] = c.referer;
                co["userAgent"] = c.userAgent;
                co["isFavorite"] = c.isFavorite;
                co["orderIndex"] = c.orderIndex;
                channelsArr.append(co);
            }
            go["channels"] = channelsArr;
            groupsArr.append(go);
        }
        po["groups"] = groupsArr;
        playlistsArr.append(po);
    }

    QJsonObject root;
    root["app"] = QStringLiteral("M3uVideoPlayerPc");
    root["backupVersion"] = 1;
    root["settings"] = QJsonObject::fromVariantMap(m_settings->toMap());
    root["playlists"] = playlistsArr;

    QSaveFile file(toLocalPath(fileUrl));
    if (!file.open(QIODevice::WriteOnly)) {
        emit backupFinished(false, QStringLiteral("Cannot write file"));
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        emit backupFinished(false, QStringLiteral("Failed to save file"));
        return false;
    }
    emit backupFinished(true, QStringLiteral("Backup saved"));
    return true;
}

bool AppController::importBackup(const QString& fileUrl) {
    if (!m_playlistRepo || !m_channelRepo) {
        emit restoreFinished(false, QStringLiteral("App not initialized"));
        return false;
    }

    QFile file(toLocalPath(fileUrl));
    if (!file.open(QIODevice::ReadOnly)) {
        emit restoreFinished(false, QStringLiteral("Cannot open file"));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit restoreFinished(false, QStringLiteral("Not a valid backup file"));
        return false;
    }
    const QJsonObject root = doc.object();
    if (root.value("app").toString() != QLatin1String("M3uVideoPlayerPc")) {
        emit restoreFinished(false, QStringLiteral("Not a M3U Player backup file"));
        return false;
    }

    // Replace-all restore: existing playlists (and their content, via
    // ON DELETE CASCADE) are removed first so the app matches the backup.
    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        m_playlistRepo->deletePlaylist(p.id);
    }

    int restored = 0;
    for (const QJsonValue& pv : root.value("playlists").toArray()) {
        const QJsonObject po = pv.toObject();
        Domain::Playlist playlist;
        playlist.name = po.value("name").toString();
        playlist.url = po.value("url").toString();
        playlist.username = po.value("username").toString();
        playlist.password = po.value("password").toString();
        playlist.updateFrequency =
            static_cast<Domain::UpdateFrequency>(po.value("updateFrequency").toInt());
        playlist.lastUpdated = QDateTime::fromString(po.value("lastUpdated").toString(), Qt::ISODate);
        playlist.createdAt = QDateTime::fromString(po.value("createdAt").toString(), Qt::ISODate);
        if (playlist.name.isEmpty() || !m_playlistRepo->insertPlaylist(playlist)) continue;

        for (const QJsonValue& gv : po.value("groups").toArray()) {
            const QJsonObject go = gv.toObject();
            Domain::Group group;
            group.playlistId = playlist.id;
            group.name = go.value("name").toString();
            group.orderIndex = go.value("orderIndex").toInt();
            if (!m_channelRepo->insertGroup(group)) continue;

            std::vector<Domain::Channel> channels;
            for (const QJsonValue& cv : go.value("channels").toArray()) {
                const QJsonObject co = cv.toObject();
                Domain::Channel c;
                c.playlistId = playlist.id;
                c.groupId = group.id;
                c.name = co.value("name").toString();
                c.streamUrl = co.value("streamUrl").toString();
                c.logoUrl = co.value("logoUrl").toString();
                c.type = static_cast<Domain::ContentType>(co.value("type").toInt());
                c.tvgId = co.value("tvgId").toString();
                c.tvgName = co.value("tvgName").toString();
                c.tvgShift = co.value("tvgShift").toString();
                c.referer = co.value("referer").toString();
                c.userAgent = co.value("userAgent").toString();
                c.isFavorite = co.value("isFavorite").toBool();
                c.orderIndex = co.value("orderIndex").toInt();
                channels.push_back(c);
            }
            m_channelRepo->insertChannels(channels);
        }
        ++restored;
    }

    if (root.contains("settings")) {
        m_settings->applyMap(root.value("settings").toObject().toVariantMap());
    }

    m_playlistViewModel->loadPlaylists();
    emit restoreFinished(true, QStringLiteral("Restored %1 playlist(s)").arg(restored));
    return true;
}

bool AppController::exportHistoryM3u(const QString& fileUrl) {
    if (!m_playlistRepo || !m_channelRepo) {
        emit historyExportFinished(false, QStringLiteral("App not initialized"));
        return false;
    }

    int historyId = -1;
    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        if (p.name == QLatin1String("History")) { historyId = p.id; break; }
    }
    if (historyId < 0) {
        emit historyExportFinished(false, QStringLiteral("No history to export"));
        return false;
    }

    // group id -> name so each entry keeps its group-title
    QHash<int, QString> groupNames;
    for (const auto& g : m_channelRepo->getGroupsByPlaylistId(historyId)) {
        groupNames[g.id] = g.name;
    }

    const auto channels = m_channelRepo->getChannelsByPlaylistId(historyId);
    if (channels.empty()) {
        emit historyExportFinished(false, QStringLiteral("History is empty"));
        return false;
    }

    QString out;
    out += "#EXTM3U\n";
    for (const auto& c : channels) {
        QString attrs;
        const QString group = groupNames.value(c.groupId);
        if (!group.isEmpty()) attrs += QString(" group-title=\"%1\"").arg(group);
        if (!c.logoUrl.isEmpty()) attrs += QString(" tvg-logo=\"%1\"").arg(c.logoUrl);
        out += QString("#EXTINF:-1%1,%2\n").arg(attrs, c.name);
        // Same #EXTVLCOPT format the importer understands
        if (!c.referer.isEmpty()) out += "#EXTVLCOPT:http-referrer=" + c.referer + "\n";
        if (!c.userAgent.isEmpty()) out += "#EXTVLCOPT:http-user-agent=" + c.userAgent + "\n";
        out += c.streamUrl + "\n";
    }

    QSaveFile file(toLocalPath(fileUrl));
    if (!file.open(QIODevice::WriteOnly)) {
        emit historyExportFinished(false, QStringLiteral("Cannot write file"));
        return false;
    }
    file.write(out.toUtf8());
    if (!file.commit()) {
        emit historyExportFinished(false, QStringLiteral("Failed to save file"));
        return false;
    }
    emit historyExportFinished(true,
        QStringLiteral("Exported %1 item(s)").arg(channels.size()));
    return true;
}

bool AppController::importHistoryM3u(const QString& fileUrl) {
    const QString localPath = toLocalPath(fileUrl);
    if (!QFileInfo::exists(localPath)) {
        emit historyImportFinished(false, QStringLiteral("File not found"));
        return false;
    }

    Data::ParseResult parsed = Data::M3uParser::parseFile(localPath, /*playlistId=*/0);
    if (parsed.channels.empty()) {
        emit historyImportFinished(false, QStringLiteral("No entries found in the file"));
        return false;
    }

    importIntoHistory(parsed);
    emit historyImportFinished(true,
        QStringLiteral("Imported %1 item(s) into History").arg(parsed.channels.size()));
    return true;
}

bool AppController::clearHistory() {
    if (!m_playlistRepo || !m_channelRepo) {
        emit historyCleared(false, QStringLiteral("App not initialized"));
        return false;
    }

    int historyId = -1;
    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        if (p.name == QLatin1String("History")) { historyId = p.id; break; }
    }
    if (historyId < 0) {
        emit historyCleared(true, QStringLiteral("History is already empty"));
        return true;
    }

    if (!m_channelRepo->deleteContentByPlaylistId(historyId)) {
        emit historyCleared(false, QStringLiteral("Failed to clear history"));
        return false;
    }

    // Refresh any open views: group/channel lists read straight from the repo.
    if (m_groupViewModel) m_groupViewModel->loadGroups(historyId);
    emit historyCleared(true, QStringLiteral("History cleared"));
    return true;
}

bool AppController::getAndSetDemoPromptShown() {
    QSettings store;
    bool shown = store.value("App/demoPromptShown", false).toBool();
    if (!shown) {
        store.setValue("App/demoPromptShown", true);
    }
    return shown;
}

bool AppController::loadDemoData() {
    // Pass the QRC path directly to importBackup.
    // The toLocalPath helper handles "file:" urls but leaves ":/" paths untouched.
    bool success = importBackup(":/M3uVideoPlayer/src/assets/demo_data.json");
    if (success) {
        emit restoreFinished(true, QStringLiteral("Demo data loaded"));
    }
    return success;
}

// ===== Playback resume =====

void AppController::backfillContentTypes() {
    if (!m_channelRepo || !m_playlistRepo) return;

    // One-shot: mark in QSettings so old databases are upgraded exactly once.
    QSettings store;
    if (store.value("Playback/typesBackfilled", false).toBool()) return;

    int updated = 0;
    for (const auto& p : m_playlistRepo->getAllPlaylists()) {
        // group id -> name, needed as a detection signal
        QHash<int, QString> groupNames;
        for (const auto& g : m_channelRepo->getGroupsByPlaylistId(p.id)) {
            groupNames[g.id] = g.name;
        }
        for (const auto& c : m_channelRepo->getChannelsByPlaylistId(p.id)) {
            const auto detected = Data::ContentTypeDetector::detect(
                c.name, c.streamUrl, groupNames.value(c.groupId));
            if (detected != c.type) {
                Domain::Channel channel = c;
                channel.type = detected;
                if (m_channelRepo->updateChannel(channel)) ++updated;
            }
        }
    }

    store.setValue("Playback/typesBackfilled", true);
    qDebug() << "Content-type backfill done," << updated << "channel(s) updated";
}

QVariantMap AppController::getMovieResume(const QString& streamUrl) {
    QVariantMap result;
    result["found"] = false;
    if (!m_resumeRepo) return result;

    const auto point = m_resumeRepo->getMovieResume(streamUrl);
    if (!point) return result;

    result["found"] = true;
    result["positionMs"] = static_cast<qlonglong>(point->positionMs);
    result["durationMs"] = static_cast<qlonglong>(point->durationMs);
    return result;
}

void AppController::clearMovieResume(const QString& streamUrl) {
    if (m_resumeRepo) m_resumeRepo->clearMovieResume(streamUrl);
}

void AppController::saveProgress(int playlistId, int groupId, const QString& groupTitle,
                                 const QString& streamUrl, const QString& title,
                                 const QString& referer, const QString& userAgent,
                                 int contentType, qreal positionMs, qreal durationMs) {
    if (!m_resumeRepo || streamUrl.isEmpty() || playlistId <= 0) return;

    const auto type = static_cast<Domain::ContentType>(contentType);
    const bool isVod = (type == Domain::ContentType::MOVIE
                     || type == Domain::ContentType::SERIES)
                    && durationMs > 0;

    if (isVod) {
        Data::MovieResumePoint movie;
        movie.playlistId = playlistId;
        movie.groupId = groupId;
        movie.groupTitle = groupTitle;
        movie.streamUrl = streamUrl;
        movie.title = title;
        movie.positionMs = static_cast<qint64>(positionMs);
        movie.durationMs = static_cast<qint64>(durationMs);
        m_resumeRepo->saveMovieResume(movie);
    }

    Data::PlaylistResumePoint last;
    last.playlistId = playlistId;
    last.groupId = groupId;
    last.groupTitle = groupTitle;
    last.streamUrl = streamUrl;
    last.title = title;
    last.referer = referer;
    last.userAgent = userAgent;
    last.contentType = contentType;
    // Live TV: remember the channel only, never a time position.
    last.positionMs = isVod ? static_cast<qint64>(positionMs) : 0;
    last.durationMs = isVod ? static_cast<qint64>(durationMs) : 0;
    m_resumeRepo->savePlaylistResume(last);
}

QVariantMap AppController::getPlaylistResume(int playlistId) {
    QVariantMap result;
    result["found"] = false;
    if (!m_resumeRepo) return result;

    const auto point = m_resumeRepo->getPlaylistResume(playlistId);
    if (!point) return result;

    result["found"] = true;
    result["playlistId"] = point->playlistId;
    result["groupId"] = point->groupId;
    result["groupTitle"] = point->groupTitle;
    result["streamUrl"] = point->streamUrl;
    result["title"] = point->title;
    result["referer"] = point->referer;
    result["userAgent"] = point->userAgent;
    result["contentType"] = point->contentType;
    result["positionMs"] = static_cast<qlonglong>(point->positionMs);
    result["durationMs"] = static_cast<qlonglong>(point->durationMs);
    return result;
}

int AppController::channelTypeForUrl(const QString& streamUrl) {
    if (m_channelViewModel) {
        const int idx = m_channelViewModel->findIndexByUrl(streamUrl);
        if (idx >= 0) {
            return m_channelViewModel->channelType(idx);
        }
    }
    return static_cast<int>(Domain::ContentType::UNKNOWN);
}


