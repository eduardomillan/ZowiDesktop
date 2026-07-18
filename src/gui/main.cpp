#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocale>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>
#include <QTextStream>
#include <QQuickStyle>
#ifdef QT_DEBUG
#include <QFileSystemWatcher>
#include <QDirIterator>
#endif

#include "controllers/TranslatorController.h"
#include "controllers/SessionController.h"
#include "controllers/BluetoothController.h"
#include "controllers/ConfigController.h"

static QQmlApplicationEngine *s_engine = nullptr;
static QString s_qmlPath;

static void reloadQml()
{
    if (!s_engine) return;
    auto roots = s_engine->rootObjects();
    for (auto *obj : roots)
        obj->deleteLater();
    s_engine->clearComponentCache();
    s_engine->load(s_qmlPath);
}

#ifdef QT_DEBUG
static void watchQmlDir(QFileSystemWatcher &watcher, const QString &dir)
{
    watcher.addPath(dir);
    QDirIterator it(dir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
        watcher.addPath(it.next());
}
#endif

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);
    app.setApplicationName("ZowiDesktop");
    app.setOrganizationName("ZowiDesktop");
    app.setWindowIcon(QIcon(":/images/app_icon.png"));

    QString appDir = QCoreApplication::applicationDirPath();
    app.addLibraryPath(appDir);

    // --- Controllers (wrapping core library) ---
    TranslatorController translator;
    SessionController session;
    BluetoothController bluetooth;
    ConfigController config;
    bluetooth.setSessionController(&session);

    QString locale = QLocale::system().name();
    QStringList supported = translator.availableLocales();
    if (!supported.contains(locale))
        locale = "en_US";
    translator.load(locale);

    // --- QML engine ---
    QQmlApplicationEngine engine;
    s_engine = &engine;

    engine.addImportPath(appDir + "/Qt/qml");

    engine.rootContext()->setContextProperty("Session", &session);
    engine.rootContext()->setContextProperty("Translator", &translator);
    engine.rootContext()->setContextProperty("Bluetooth", &bluetooth);
    engine.rootContext()->setContextProperty("Config", &config);

#ifdef QT_DEBUG
    s_qmlPath = QUrl::fromLocalFile("src/views/main.qml").toString();
#else
    s_qmlPath = "qrc:/src/views/main.qml";
#endif

    QObject::connect(&translator, &TranslatorController::languageChanged,
                     &reloadQml);

    reloadQml();

#ifdef QT_DEBUG
    QFileSystemWatcher watcher;
    watchQmlDir(watcher, "src/views");
    QObject::connect(&watcher, &QFileSystemWatcher::directoryChanged,
                     [&watcher](const QString &path) {
        Q_UNUSED(path)
        watchQmlDir(watcher, "src/views");
    });
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged,
                     [](const QString &path) {
        reloadQml();
    });
#endif

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
