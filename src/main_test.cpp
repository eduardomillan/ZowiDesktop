#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qDebug() << "Starting...";
    QQmlApplicationEngine engine;
    qDebug() << "Creating engine...";
    engine.load(QUrl("qrc:/qml/main.qml"));
    qDebug() << "Loaded QML";
    if (engine.rootObjects().isEmpty()) {
        qDebug() << "No root objects!";
        return -1;
    }
    qDebug() << "Success";
    return app.exec();
}
