#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtSql/QSqlDatabase>
#include <QDebug>
#include <QFileInfo>
#include <QIcon>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <QLocalServer>
#include <QLocalSocket>

#include <QQuickWindow>
#include <QQuickStyle>
#include "infrastructure/player/MpvVideoItem.h"
#include "ui/AppController.h"
#include "ui/ChannelViewModel.h"

// Name of the local socket used for single-instance detection: a second
// launch (e.g. double-clicking another .m3u file) forwards its file path to
// the running instance and exits instead of opening a second window.
static const char* kInstanceServerName = "M3uVideoPlayerPc-instance";

#ifdef Q_OS_WIN
// Register .m3u/.m3u8 so double-clicking a playlist file opens this app.
// Writes to HKEY_CURRENT_USER\Software\Classes — per-user, no admin rights
// needed. Safe to run on every startup (idempotent).
static void registerM3uFileAssociation()
{
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString progId = "M3uVideoPlayer.Playlist";

    QSettings classes("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);

    QString appDir = QFileInfo(appPath).absolutePath();
    QString iconPath = QDir::toNativeSeparators(appDir + "/favicon.ico");

    // ProgID with the open command
    classes.setValue(progId + "/.", "M3U Playlist");
    classes.setValue(progId + "/DefaultIcon/.", "\"" + appPath + "\",0");
    classes.setValue(progId + "/shell/open/command/.", "\"" + appPath + "\" \"%1\"");

    // Associate the extensions with the ProgID (OpenWithProgids keeps the
    // user's existing default intact while adding us to "Open with").
    for (const QString& ext : {QString(".m3u"), QString(".m3u8")}) {
        if (classes.value(ext + "/.").toString().isEmpty()) {
            classes.setValue(ext + "/.", progId);
        }
        classes.setValue(ext + "/OpenWithProgids/" + progId, "");
        
        // Register under Applications to explicitly tell Windows we support these extensions
        classes.setValue("Applications/M3uVideoPlayerPc.exe/SupportedTypes/" + ext, "");
    }
    
    // Set the open command for the application itself
    classes.setValue("Applications/M3uVideoPlayerPc.exe/shell/open/command/.", "\"" + appPath + "\" \"%1\"");

    // Set FriendlyAppName and Icon so the "Open With" dialog looks nice
    classes.setValue("Applications/M3uVideoPlayerPc.exe/FriendlyAppName", "M3U Video Player PC");
    classes.setValue("Applications/M3uVideoPlayerPc.exe/DefaultIcon/.", "\"" + appPath + "\",0");
}

// Add the app to the "Open with" list for local video files (mkv/mp4/...).
// Unlike .m3u, we never claim the extension's default handler — we only
// register under OpenWithProgids/SupportedTypes so the user can pick us.
static void registerVideoFileAssociation()
{
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString progId = "M3uVideoPlayer.Video";

    QSettings classes("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);

    classes.setValue(progId + "/.", "Video File");
    classes.setValue(progId + "/DefaultIcon/.", "\"" + appPath + "\",0");
    classes.setValue(progId + "/shell/open/command/.", "\"" + appPath + "\" \"%1\"");

    for (const QString& extName : ChannelViewModel::videoFileExtensions()) {
        const QString ext = "." + extName;
        classes.setValue(ext + "/OpenWithProgids/" + progId, "");
        classes.setValue("Applications/M3uVideoPlayerPc.exe/SupportedTypes/" + ext, "");
    }
}
#endif

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");

    // Force OpenGL RHI backend for libmpv mpv_render_context
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/M3uVideoPlayer/src/assets/icon-512.png"));

    // Application Info
    app.setOrganizationName("M3UPlayerTeam");
    app.setOrganizationDomain("m3uplayer.local");
    app.setApplicationName("M3U Video Player Desktop");
    app.setApplicationVersion("1.0.0");

    // Extract a playable file path passed on the command line (double-click /
    // "Open with"), if any: either an .m3u/.m3u8 playlist or a local video.
    QString fileToOpen;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);
        const bool isM3u = arg.endsWith(".m3u", Qt::CaseInsensitive)
                        || arg.endsWith(".m3u8", Qt::CaseInsensitive);
        if (isM3u || ChannelViewModel::isVideoFile(arg)) {
            if (QFileInfo::exists(arg)) {
                fileToOpen = QFileInfo(arg).absoluteFilePath();
            }
            break;
        }
    }

    // Single-instance: if another instance is already running, hand the file
    // over to it and exit — don't open a second window.
    {
        QLocalSocket probe;
        probe.connectToServer(kInstanceServerName);
        if (probe.waitForConnected(300)) {
            probe.write(fileToOpen.toUtf8()); // empty payload = just "activate"
            probe.flush();
            probe.waitForBytesWritten(1000);
            probe.disconnectFromServer();
            return 0;
        }
    }

#ifdef Q_OS_WIN
    registerM3uFileAssociation();
    registerVideoFileAssociation();
#endif

    // Init AppController
    AppController appController;
    appController.init();

    // Become the single-instance server: accept file paths forwarded by
    // later launches and import them into this running instance.
    QLocalServer instanceServer;
    QLocalServer::removeServer(kInstanceServerName); // clear stale socket after a crash
    if (instanceServer.listen(kInstanceServerName)) {
        QObject::connect(&instanceServer, &QLocalServer::newConnection, &appController, [&]() {
            QLocalSocket* client = instanceServer.nextPendingConnection();
            if (!client) return;
            QObject::connect(client, &QLocalSocket::readyRead, &appController, [&appController, client]() {
                const QString path = QString::fromUtf8(client->readAll()).trimmed();
                if (!path.isEmpty()) {
                    if (ChannelViewModel::isVideoFile(path)) {
                        appController.openLocalVideo(path);
                    } else {
                        appController.openM3uFile(path);
                    }
                }
                // Raise the existing window either way
                const auto windows = QGuiApplication::topLevelWindows();
                if (!windows.isEmpty()) {
                    QWindow* win = windows.first();
                    win->show();
                    win->raise();
                    win->requestActivate();
                }
                client->deleteLater();
            });
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        });
    } else {
        qWarning() << "Single-instance server failed to listen:" << instanceServer.errorString();
    }

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

    // If launched with a playable file (double-click / "Open with"), open it
    // once the UI is up: .m3u/.m3u8 imports into History, local videos play
    // directly without touching History.
    if (!fileToOpen.isEmpty()) {
        QTimer::singleShot(0, &appController, [&appController, fileToOpen]() {
            if (ChannelViewModel::isVideoFile(fileToOpen)) {
                appController.openLocalVideo(fileToOpen);
            } else {
                appController.openM3uFile(fileToOpen);
            }
        });
    }

    return app.exec();
}
