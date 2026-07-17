#include "AppController.h"
#include <QDebug>
#include <QGuiApplication>
#include <QWindow>
#include <QScreen>
#include "../infrastructure/TrustedTimeSource.h"
#include "PlaylistViewModel.h"
#include "GroupViewModel.h"
#include "ChannelViewModel.h"
#include "../data/DatabaseManager.h"
#include "../data/PlaylistRepository.h"
#include "../data/ChannelRepository.h"

static AppController *s_instance = nullptr;

AppController::AppController(QObject *parent) 
    : QObject(parent), 
      m_dbManager(new Data::DatabaseManager(this)),
      m_timeSource(new Infrastructure::TrustedTimeSource(this)),
      m_syncTimer(new QTimer(this))
{
    s_instance = this;
    
    // Setup background sync timer (e.g. check every hour)
    connect(m_syncTimer, &QTimer::timeout, this, &AppController::onSyncTimerFired);
    m_syncTimer->setInterval(60 * 60 * 1000); // 1 hour
}

AppController::~AppController() {
    if (m_channelViewModel) delete m_channelViewModel;
    if (m_groupViewModel) delete m_groupViewModel;
    if (m_playlistViewModel) delete m_playlistViewModel;
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
        m_playlistViewModel = new PlaylistViewModel(m_playlistRepo, m_channelRepo, this);
        m_groupViewModel = new GroupViewModel(m_channelRepo, this);
        m_channelViewModel = new ChannelViewModel(m_channelRepo, this);
    } else {
        qWarning() << "Database initialization failed!";
    }
    
    // Trigger initial on-start background sync
    triggerBackgroundSync();
    
    // Start periodic timer
    m_syncTimer->start();
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

void AppController::onSyncTimerFired() {
    qDebug() << "Triggering background sync...";
    emit syncStarted();
    
    // Fetch trusted network time first
    m_timeSource->fetchNetworkTime([this](bool success) {
        if (success) {
            qDebug() << "Trusted network time fetched. Current UTC:" << m_timeSource->getCurrentTime();
        } else {
            qWarning() << "Failed to fetch trusted network time. Falling back to system clock.";
        }
        
        // TODO: Iterate over playlists and check updateFrequency
        // Then run M3uParser or XtreamApiClient as needed
        // For now, simulate success
        
        qDebug() << "Background sync finished.";
        emit syncFinished(true);
    });
}

