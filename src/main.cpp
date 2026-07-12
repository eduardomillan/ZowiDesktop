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
#ifdef QT_DEBUG
#include <QFileSystemWatcher>
#include <QDirIterator>
#endif

#include "services/TranslationEngine.h"
#include "services/SessionService.h"
#include "services/BluetoothService.h"
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
    QGuiApplication app(argc, argv);
    app.setApplicationName("ZowiDesktop");
    app.setOrganizationName("ZowiDesktop");
    app.setWindowIcon(QIcon(":/images/app_icon.png"));

    QString appDir = QCoreApplication::applicationDirPath();

    // Portable mode: register exe directory as library/plugin path
    app.addLibraryPath(appDir);

    // --- Services ---
    TranslationEngine translationEngine;
    SessionService sessionService;
    BluetoothService bluetoothService;

    QString locale = QLocale::system().name();
    QStringList supported = translationEngine.availableLocales();
    if (!supported.contains(locale))
        locale = "en_US";
    translationEngine.load(locale);

    // --- Controllers ---
    TranslatorController translator(&translationEngine);
    SessionController session(&sessionService);
    BluetoothController bluetooth(&bluetoothService);
    ConfigController config;

    // --- QML engine ---
    QQmlApplicationEngine engine;
    s_engine = &engine;

    // Portable mode: add QML import path
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

    // Write diagnostics to Desktop
    QString diagPath = QDir::homePath() + "/Desktop/zowi_debug.log";
    QFile diagFile(diagPath);
    QTextStream ts(&diagFile);
    if (diagFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ts << "appDir=" << appDir << "\n";
        ts << "platforms exists=" << QFile::exists(appDir + "/platforms/qwindows.dll") << "\n";
        ts << "libraryPaths=" << app.libraryPaths().size() << "\n";
        for (const QString &p : app.libraryPaths())
            ts << "  lib: " << p << "\n";

        QDir appDirObj(appDir);
        ts << "DLLs in appDir:\n";
        for (const QString &f : appDirObj.entryList(QStringList() << "*.dll"))
            ts << "  " << f << "\n";

        ts << "importPaths=" << engine.importPathList().size() << "\n";
        for (const QString &p : engine.importPathList())
            ts << "  import: " << p << "\n";

        QDir qmlDir(appDir + "/Qt/qml");
        if (qmlDir.exists()) {
            ts << "QML modules in Qt/qml:\n";
            for (const QString &d : qmlDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot))
                ts << "  " << d << "\n";
        } else {
            ts << "Qt/qml does NOT exist\n";
        }
    }

    reloadQml();

    if (ts.device()) {
        ts << "rootObjects=" << engine.rootObjects().size() << "\n";
        if (!engine.rootObjects().isEmpty())
            ts << "QML loaded OK\n";
        else
            ts << "QML FAILED to load\n";
        ts.flush();
        diagFile.close();
    }

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
