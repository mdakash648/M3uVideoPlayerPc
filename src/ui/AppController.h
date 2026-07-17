#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include "../infrastructure/TrustedTimeSource.h"
#include "../data/DatabaseManager.h"
#include "PlaylistViewModel.h"
#include "GroupViewModel.h"
#include "ChannelViewModel.h"

// namespace UI removed for QML registrar compatibility
class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(PlaylistViewModel* playlistViewModel READ playlistViewModel CONSTANT)
    Q_PROPERTY(GroupViewModel* groupViewModel READ groupViewModel CONSTANT)
    Q_PROPERTY(ChannelViewModel* channelViewModel READ channelViewModel CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    static AppController* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    Q_INVOKABLE void init();
    Q_INVOKABLE void triggerBackgroundSync();
    Q_INVOKABLE void restoreMaximized();

    PlaylistViewModel* playlistViewModel() const { return m_playlistViewModel; }
    GroupViewModel* groupViewModel() const { return m_groupViewModel; }
    ChannelViewModel* channelViewModel() const { return m_channelViewModel; }

signals:
    void syncStarted();
    void syncFinished(bool success);

private:
    void onSyncTimerFired();

    Data::DatabaseManager* m_dbManager;
    Infrastructure::TrustedTimeSource* m_timeSource;
    QTimer* m_syncTimer;

    Data::PlaylistRepository* m_playlistRepo = nullptr;
    Data::ChannelRepository* m_channelRepo = nullptr;
    PlaylistViewModel* m_playlistViewModel = nullptr;
    GroupViewModel* m_groupViewModel = nullptr;
    ChannelViewModel* m_channelViewModel = nullptr;
};

