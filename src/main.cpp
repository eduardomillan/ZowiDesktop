#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocale>
#include <QIcon>
#include "translator.h"
#include "sessioncontroller.h"
#include "zowibluetoothcontroller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("ZowiDesktop");
    app.setOrganizationName("ZowiDesktop");
    app.setWindowIcon(QIcon(":/images/app_icon.png"));

    SessionController session;
    Translator translator;
    QString locale = QLocale::system().name();
    QStringList supported = translator.availableLocales();
    if (!supported.contains(locale))
        locale = "en_US";
    translator.load(locale);

    QQmlApplicationEngine engine;
    ZowiBluetoothController bluetooth;
    engine.rootContext()->setContextProperty("Session", &session);
    engine.rootContext()->setContextProperty("Translator", &translator);
    engine.rootContext()->setContextProperty("Bluetooth", &bluetooth);

    engine.load(QUrl("qrc:/qml/main.qml"));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
