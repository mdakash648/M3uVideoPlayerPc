#include "AppController.h"
#include <QDebug>

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
    } else {
        qWarning() << "Database initialization failed!";
    }
    
    // Trigger initial on-start background sync
    triggerBackgroundSync();
    
    // Start periodic timer
    m_syncTimer->start();
}

void AppController::triggerBackgroundSync() {
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

void AppController::onSyncTimerFired() {
    triggerBackgroundSync();
}

