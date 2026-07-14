#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtSql/QSqlDatabase>
#include <QDebug>

// Forward declarations for DI / ViewModels
// #include "ui/MainViewModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // Application Info
    app.setOrganizationName("M3UPlayerTeam");
    app.setOrganizationDomain("m3uplayer.local");
    app.setApplicationName("M3U Video Player Desktop");
    app.setApplicationVersion("1.0.0");

    // Dependency Injection Setup (Manual for now)
    // auto database = QSqlDatabase::addDatabase("QSQLITE");
    // database.setDatabaseName("m3uplayer.db");
    
    // auto playlistRepo = std::make_shared<PlaylistRepository>(database);
    // auto channelRepo = std::make_shared<ChannelRepository>(database);
    
    // auto mainViewModel = std::make_shared<MainViewModel>(playlistRepo, channelRepo);

    QQmlApplicationEngine engine;
    
    // Expose ViewModels to QML
    // engine.rootContext()->setContextProperty("mainViewModel", mainViewModel.get());
    
    // Global properties or Enums
    // qmlRegisterType<Enums>("M3uVideoPlayer", 1, 0, "AppEnums");

    const QUrl url(u"qrc:/M3uVideoPlayer/src/ui/main.qml"_qs);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
