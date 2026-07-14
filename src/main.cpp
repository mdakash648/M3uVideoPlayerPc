#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtSql/QSqlDatabase>
#include <QDebug>

#include <QQuickWindow>
#include <QQuickStyle>
#include "infrastructure/player/MpvVideoItem.h"
#include "ui/AppController.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");

    // Force OpenGL RHI backend for libmpv mpv_render_context
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
    
    // Application Info
    app.setOrganizationName("M3UPlayerTeam");
    app.setOrganizationDomain("m3uplayer.local");
    app.setApplicationName("M3U Video Player Desktop");
    app.setApplicationVersion("1.0.0");

    // Init AppController
    AppController appController;
    appController.init();

    // Register MpvVideoItem for QML
    qmlRegisterType<MpvVideoItem>("M3uVideoPlayer", 1, 0, "MpvVideoItem");

    QQmlApplicationEngine engine;
    
    // Expose ViewModels to QML
    qmlRegisterSingletonInstance("M3uVideoPlayer", 1, 0, "AppController", &appController);
    
    // Global properties or Enums
    // qmlRegisterType<Enums>("M3uVideoPlayer", 1, 0, "AppEnums");

    const QUrl url("qrc:/M3uVideoPlayer/src/ui/main.qml");
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
