#include <QCoreApplication>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QDebug>
#include <QTimer>

static const QBluetoothUuid SPP_UUID = QBluetoothUuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Discovery
    QBluetoothDeviceDiscoveryAgent *agent = new QBluetoothDeviceDiscoveryAgent(&app);
    QObject::connect(agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        [](const QBluetoothDeviceInfo &device) {
            if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
                return;
            qDebug() << "FOUND:" << device.name() << device.address().toString();
        });
    QObject::connect(agent, &QBluetoothDeviceDiscoveryAgent::finished,
        [&]() {
            qDebug() << "Scan finished";
            QCoreApplication::quit();
        });
    QObject::connect(agent,
        QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
        [agent](QBluetoothDeviceDiscoveryAgent::Error) {
            qDebug() << "Scan error:" << agent->errorString();
            QCoreApplication::quit();
        });

    qDebug() << "Starting Bluetooth scan (5s)...";
    agent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);

    QTimer::singleShot(5000, [&]() {
        qDebug() << "Timeout - stopping scan";
        agent->stop();
        QCoreApplication::quit();
    });

    return app.exec();
}
