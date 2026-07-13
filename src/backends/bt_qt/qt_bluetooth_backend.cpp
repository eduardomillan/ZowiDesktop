#include "qt_bluetooth_backend.h"
#include <QDebug>
#include <QBluetoothAddress>
#include <QBluetoothUuid>
#include <QBluetoothServiceInfo>

static const QBluetoothUuid SPP_UUID = QBluetoothUuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));

namespace zowi {

QtBluetoothBackend::QtBluetoothBackend(QObject *parent)
    : QObject(parent)
{
}

QtBluetoothBackend::~QtBluetoothBackend()
{
    stopDiscovery();
    QtBluetoothBackend::disconnect();
}

bool QtBluetoothBackend::init()
{
    return true;
}

void QtBluetoothBackend::startDiscovery()
{
    if (m_scanning)
        return;

    if (m_discoveryAgent) {
        m_discoveryAgent->deleteLater();
        m_discoveryAgent = nullptr;
    }

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    QObject::connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                     this, &QtBluetoothBackend::onDeviceDiscovered);
    QObject::connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
                     this, &QtBluetoothBackend::onScanFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
                     this, &QtBluetoothBackend::onScanError);
#else
    QObject::connect(m_discoveryAgent,
        QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
        this, &QtBluetoothBackend::onScanError);
#endif

    m_discoveredDevices.clear();
    m_scanning = true;
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
}

void QtBluetoothBackend::stopDiscovery()
{
    if (m_discoveryAgent && m_scanning) {
        m_discoveryAgent->stop();
        m_scanning = false;
    }
}

bool QtBluetoothBackend::connect(const std::string &address)
{
    m_deviceAddress = QString::fromStdString(address);
    m_deviceName = m_discoveredDevices.value(m_deviceAddress);

    if (m_socket) {
        m_socket->disconnectFromService();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    m_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    QObject::connect(m_socket, &QBluetoothSocket::connected,
                     this, &QtBluetoothBackend::onSocketConnected);
    QObject::connect(m_socket, &QBluetoothSocket::disconnected,
                     this, &QtBluetoothBackend::onSocketDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(m_socket, &QBluetoothSocket::errorOccurred,
                     this, &QtBluetoothBackend::onSocketError);
#else
    QObject::connect(m_socket,
        QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
        this, &QtBluetoothBackend::onSocketError);
#endif
    QObject::connect(m_socket, &QBluetoothSocket::readyRead,
                     this, &QtBluetoothBackend::onDataReady);

    m_socket->connectToService(QBluetoothAddress(m_deviceAddress), SPP_UUID, QIODevice::ReadWrite);
    return true;
}

void QtBluetoothBackend::disconnect()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }

    m_deviceAddress.clear();
    m_deviceName.clear();

    if (m_socket) {
        m_socket->disconnectFromService();
    }

    if (m_reconnectTimer) {
        m_reconnectTimer->deleteLater();
        m_reconnectTimer = nullptr;
    }
}

bool QtBluetoothBackend::send(const std::string &data)
{
    if (!m_socket || m_socket->state() != QBluetoothSocket::SocketState::ConnectedState) {
        m_lastError = "Not connected to any device";
        if (m_onError) m_onError(m_lastError);
        return false;
    }

    QByteArray bytes = QString::fromStdString(data).toUtf8();
    m_socket->write(bytes);
    return true;
}

bool QtBluetoothBackend::isConnected() const
{
    return m_socket && m_socket->state() == QBluetoothSocket::SocketState::ConnectedState;
}

std::string QtBluetoothBackend::lastError() const
{
    return m_lastError;
}

QString QtBluetoothBackend::deviceName() const
{
    return m_deviceName;
}

QString QtBluetoothBackend::deviceAddress() const
{
    return m_deviceAddress;
}

bool QtBluetoothBackend::isScanning() const
{
    return m_scanning;
}

void QtBluetoothBackend::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
        return;

    QString name = device.name().trimmed();
    if (name.isEmpty())
        name = "(unknown)";

    QString addr = device.address().toString();
    m_discoveredDevices.insert(addr, name);

    DeviceInfo info;
    info.name = name.toStdString();
    info.address = addr.toStdString();

    if (m_onDevice) m_onDevice(info);
}

void QtBluetoothBackend::onScanFinished()
{
    m_scanning = false;
}

void QtBluetoothBackend::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error)
    m_scanning = false;
    QString msg = m_discoveryAgent ? m_discoveryAgent->errorString() : QStringLiteral("Scan error");
    m_lastError = msg.toStdString();
    if (m_onError) m_onError(m_lastError);
}

void QtBluetoothBackend::onSocketConnected()
{
    if (m_deviceName.isEmpty())
        m_deviceName = m_discoveredDevices.value(m_deviceAddress, m_deviceAddress);

    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }

    if (m_onConnect) m_onConnect(true);
}

void QtBluetoothBackend::onSocketDisconnected()
{
    if (m_onConnect) m_onConnect(false);
    startReconnectTimer();
}

void QtBluetoothBackend::onSocketError(QBluetoothSocket::SocketError error)
{
    Q_UNUSED(error)
    QString msg = m_socket ? m_socket->errorString() : QStringLiteral("Connection error");
    m_lastError = msg.toStdString();
    if (m_onError) m_onError(m_lastError);
}

void QtBluetoothBackend::onDataReady()
{
    if (!m_socket)
        return;

    QByteArray data = m_socket->readAll();
    if (m_onData) m_onData(QString::fromUtf8(data).toStdString());
}

void QtBluetoothBackend::startReconnectTimer()
{
    if (!m_deviceAddress.isEmpty()) {
        if (!m_reconnectTimer) {
            m_reconnectTimer = new QTimer(this);
            QObject::connect(m_reconnectTimer, &QTimer::timeout,
                             this, &QtBluetoothBackend::reconnectTimerTick);
        }
        m_reconnectTimer->start(3000);
    }
}

void QtBluetoothBackend::reconnectTimerTick()
{
    if (m_socket && m_socket->state() != QBluetoothSocket::SocketState::ConnectedState) {
        m_socket->connectToService(QBluetoothAddress(m_deviceAddress), SPP_UUID, QIODevice::ReadWrite);
    }
}

} // namespace zowi
