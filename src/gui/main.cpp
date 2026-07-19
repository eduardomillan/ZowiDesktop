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
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QQuickStyle>
#ifdef QT_DEBUG
#include <QFileSystemWatcher>
#include <QDirIterator>
#endif

// --- Logging to a daily file ------------------------------------------------
// All qDebug/qInfo/qWarning/qCritical output is forwarded both to stderr and to
// a per-day log file under the platform's AppData location, so users/developers
// can inspect what happened across runs. Append mode preserves past runs.
namespace {
QMutex g_logMutex;
QFile   g_logFile;

void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    const char *level = "DEBUG";
    if (type == QtInfoMsg)            level = "INFO";
    else if (type == QtWarningMsg)    level = "WARN";
    else if (type == QtCriticalMsg)   level = "ERROR";
    else if (type == QtFatalMsg)     level = "FATAL";

    QString category = ctx.category && ctx.category[0]
                       ? QString::fromUtf8(ctx.category) : QString();
    QString line = QStringLiteral("[%1] %2%3 %4")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
            .arg(QString::fromUtf8(level))
            .arg(category.isEmpty() ? QString() : " (" + category + ")")
            .arg(msg);

    // Mirror to stderr so the terminal still shows output.
    fprintf(stderr, "%s\n", line.toUtf8().constData());
    fflush(stderr);

    QMutexLocker lock(&g_logMutex);
    if (g_logFile.isOpen()) {
        g_logFile.write((line + QStringLiteral("\n")).toUtf8());
        g_logFile.flush();
    }
}

QString openLogFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!QDir().mkpath(dir))
        return QString();
    // One file per day, e.g. ZowiDesktop-2026-07-19.log
    const QString path = dir + "/ZowiDesktop-" +
            QDate::currentDate().toString("yyyy-MM-dd") + ".log";
    g_logFile.setFileName(path);
    // Append (preserve previous runs); open for read/write, create if missing.
    if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return QString();
    return path;
}
} // namespace

#include "controllers/TranslatorController.h"
#include "controllers/SessionController.h"
#include "controllers/RobotController.h"
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
    // Install the log handler first so every message (including ones emitted
    // during setup below) is captured to the daily file.
    const QString logPath = openLogFile();
    qInstallMessageHandler(messageHandler);
    qAddPostRoutine([]() {
        QMutexLocker lock(&g_logMutex);
        if (g_logFile.isOpen()) {
            g_logFile.flush();
            g_logFile.close();
        }
    });
    if (logPath.isEmpty())
        qWarning() << "Logging: could not open daily log file under AppDataLocation";
    else
        qInfo() << "ZowiDesktop started; logging to" << logPath;

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
    RobotController robot;
    ConfigController config;
    robot.setSessionController(&session);

    QString locale = session.getString("locale", "");
    if (locale.isEmpty()) {
        locale = QLocale::system().name();
        QStringList supported = translator.availableLocales();
        if (!supported.contains(locale))
            locale = "en_US";
        session.saveString("locale", locale);
    }
    translator.load(locale);

    // --- QML engine ---
    QQmlApplicationEngine engine;
    s_engine = &engine;

    engine.addImportPath(appDir + "/Qt/qml");

    engine.rootContext()->setContextProperty("Session", &session);
    engine.rootContext()->setContextProperty("Translator", &translator);
    engine.rootContext()->setContextProperty("Robot", &robot);
    engine.rootContext()->setContextProperty("Config", &config);
    engine.rootContext()->setContextProperty("AppVersion", QString(ZOWI_VERSION));

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
