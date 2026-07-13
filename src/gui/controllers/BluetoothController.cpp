#include "BluetoothController.h"

#ifdef Q_OS_WIN
#include "qt_bluetooth_backend.h"
#else
#include "qt_bluetooth_backend.h"
#endif

BluetoothController::BluetoothController(QObject *parent)
    : QObject(parent)
{
    auto backend = std::make_unique<zowi::QtBluetoothBackend>();

    backend->onDeviceFound([this](const zowi::DeviceInfo &info) {
        emit deviceDiscovered(
            QString::fromStdString(info.name),
            QString::fromStdString(info.address));
    });

    backend->onConnectionChanged([this](bool connected) {
        m_connected = connected;
        emit connectionChanged();
        if (!connected) {
            m_deviceName.clear();
            m_deviceAddress.clear();
            emit deviceChanged();
        }
    });

    backend->onDataReceived([this](const std::string &data) {
        emit dataReceived(QString::fromStdString(data));
    });

    backend->onError([this](const std::string &msg) {
        emit errorOccurred(QString::fromStdString(msg));
    });

    m_backend = std::move(backend);
}

bool BluetoothController::isConnected() const
{
    return m_backend && m_backend->isConnected();
}

bool BluetoothController::isScanning() const
{
    return m_scanning;
}

QString BluetoothController::deviceName() const
{
    return m_deviceName;
}

QString BluetoothController::deviceAddress() const
{
    return m_deviceAddress;
}

void BluetoothController::startScan()
{
    if (!m_backend) return;
    m_scanning = true;
    emit scanningChanged();
    m_backend->startDiscovery();
}

void BluetoothController::stopScan()
{
    if (!m_backend) return;
    m_backend->stopDiscovery();
    m_scanning = false;
    emit scanningChanged();
}

void BluetoothController::connectToDevice(const QString &address)
{
    if (!m_backend) return;
    m_deviceAddress = address;
    m_backend->connect(address.toStdString());
    emit deviceChanged();
}

void BluetoothController::disconnectFromDevice()
{
    if (!m_backend) return;
    m_backend->disconnect();
    m_deviceAddress.clear();
    m_deviceName.clear();
    emit deviceChanged();
}

void BluetoothController::sendData(const QString &data)
{
    if (!m_backend) return;
    m_backend->send(data.toStdString());
}
