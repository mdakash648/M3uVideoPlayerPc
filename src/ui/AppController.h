#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include "../data/DatabaseManager.h"
#include "../infrastructure/TrustedTimeSource.h"

// namespace UI removed for QML registrar compatibility
class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    static AppController* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    Q_INVOKABLE void init();
    Q_INVOKABLE void triggerBackgroundSync();

signals:
    void syncStarted();
    void syncFinished(bool success);

private:
    void onSyncTimerFired();

    Data::DatabaseManager* m_dbManager;
    Infrastructure::TrustedTimeSource* m_timeSource;
    QTimer* m_syncTimer;
};

