#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtSql/QSqlDatabase>
#include <QDebug>

#include <QQuickStyle>

#include "ui/AppController.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);
    
    // Application Info
    app.setOrganizationName("M3UPlayerTeam");
    app.setOrganizationDomain("m3uplayer.local");
    app.setApplicationName("M3U Video Player Desktop");
    app.setApplicationVersion("1.0.0");

    // Init AppController
    AppController appController;
    appController.init();

    QQmlApplicationEngine engine;
    
    // Expose ViewModels to QML
    // engine.rootContext()->setContextProperty("mainViewModel", mainViewModel.get());
    
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
