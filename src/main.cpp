#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocale>
#include <QIcon>
#include <QDebug>

#include "services/TranslationEngine.h"
#include "services/SessionService.h"
#include "services/BluetoothService.h"
#include "controllers/TranslatorController.h"
#include "controllers/SessionController.h"
#include "controllers/BluetoothController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("ZowiDesktop");
    app.setOrganizationName("ZowiDesktop");
    app.setWindowIcon(QIcon(":/images/app_icon.png"));

    // --- Services ---
    TranslationEngine translationEngine;
    SessionService sessionService;
    BluetoothService bluetoothService;

    QString locale = QLocale::system().name();
    QStringList supported = translationEngine.availableLocales();
    if (!supported.contains(locale))
        locale = "en_US";
    translationEngine.load(locale);

    // --- Controllers (thin QML wrappers) ---
    TranslatorController translator(&translationEngine);
    SessionController session(&sessionService);
    BluetoothController bluetooth(&bluetoothService);

    // --- QML engine ---
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("Session", &session);
    engine.rootContext()->setContextProperty("Translator", &translator);
    engine.rootContext()->setContextProperty("Bluetooth", &bluetooth);

#ifdef QT_DEBUG
    qDebug() << "Loading QML from filesystem (hot-reload)";
    engine.load(QUrl::fromLocalFile("src/views/main.qml"));
#else
    engine.load(QUrl("qrc:/src/views/main.qml"));
#endif

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
