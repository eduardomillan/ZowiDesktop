#include "BluetoothService.h"
#include <QDebug>

static const QBluetoothUuid SPP_UUID = QBluetoothUuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));

BluetoothService::BluetoothService(QObject *parent)
    : QObject(parent)
{
}

BluetoothService::~BluetoothService()
{
    stopScan();
    disconnectFromDevice();
}

bool BluetoothService::isConnected() const
{
    return m_socket && m_socket->state() == QBluetoothSocket::ConnectedState;
}

bool BluetoothService::isScanning() const
{
    return m_scanning;
}

QString BluetoothService::deviceName() const
{
    return m_deviceName;
}

QString BluetoothService::deviceAddress() const
{
    return m_deviceAddress;
}

QVector<DeviceInfo> BluetoothService::discoveredDevices() const
{
    return m_deviceList;
}

void BluetoothService::startScan()
{
    if (m_scanning)
        return;

    if (m_discoveryAgent) {
        m_discoveryAgent->deleteLater();
        m_discoveryAgent = nullptr;
    }

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothService::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothService::onScanFinished);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BluetoothService::onScanError);

    m_discoveredDevices.clear();
    m_deviceList.clear();
    m_scanning = true;
    emit scanningChanged();
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
}

void BluetoothService::stopScan()
{
    if (m_discoveryAgent && m_scanning) {
        m_discoveryAgent->stop();
        m_scanning = false;
        emit scanningChanged();
    }
}

void BluetoothService::connectToDevice(const QString &address)
{
    if (m_socket) {
        m_socket->disconnectFromService();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    m_deviceAddress = address;
    m_deviceName = m_discoveredDevices.value(address);

    m_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    connect(m_socket, &QBluetoothSocket::connected,
            this, &BluetoothService::onSocketConnected);
    connect(m_socket, &QBluetoothSocket::disconnected,
            this, &BluetoothService::onSocketDisconnected);
    connect(m_socket, &QBluetoothSocket::errorOccurred,
            this, &BluetoothService::onSocketError);
    connect(m_socket, &QBluetoothSocket::readyRead,
            this, &BluetoothService::onDataReady);

    qDebug() << "Connecting to" << address << "via RFCOMM SPP...";
    m_socket->connectToService(QBluetoothAddress(address), SPP_UUID, QIODevice::ReadWrite);
}

void BluetoothService::disconnectFromDevice()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_deviceAddress.clear();
    m_deviceName.clear();
    emit deviceChanged();

    if (m_socket) {
        m_socket->disconnectFromService();
    }

    if (m_reconnectTimer) {
        m_reconnectTimer->deleteLater();
        m_reconnectTimer = nullptr;
    }
}

void BluetoothService::sendData(const QString &data)
{
    if (!m_socket || m_socket->state() != QBluetoothSocket::ConnectedState) {
        emit errorOccurred(QStringLiteral("Not connected to any device"));
        return;
    }

    QByteArray bytes = data.toUtf8();
    qDebug() << "Sending:" << bytes.toHex();
    m_socket->write(bytes);
}

void BluetoothService::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
        return;

    QString name = device.name().trimmed();
    if (name.isEmpty())
        name = "(unknown)";

    m_discoveredDevices.insert(device.address().toString(), name);

    DeviceInfo info;
    info.name = name;
    info.address = device.address().toString();
    m_deviceList.append(info);

    qDebug() << "Discovered device:" << name << info.address;
    emit deviceDiscovered(info);
}

void BluetoothService::onScanFinished()
{
    m_scanning = false;
    emit scanningChanged();
    emit scanFinished();
}

void BluetoothService::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error)
    m_scanning = false;
    emit scanningChanged();
    QString msg = m_discoveryAgent ? m_discoveryAgent->errorString() : QStringLiteral("Scan error");
    emit errorOccurred(msg);
}

void BluetoothService::onSocketConnected()
{
    qDebug() << "Connected to" << m_deviceAddress;

    if (m_deviceName.isEmpty())
        m_deviceName = m_discoveredDevices.value(m_deviceAddress, m_deviceAddress);

    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }

    emit connectionChanged();
    emit deviceChanged();
}

void BluetoothService::onSocketDisconnected()
{
    qDebug() << "Disconnected from" << m_deviceAddress;
    emit connectionChanged();
    startReconnectTimer();
}

void BluetoothService::onSocketError(QBluetoothSocket::SocketError error)
{
    Q_UNUSED(error)
    QString msg = m_socket ? m_socket->errorString() : QStringLiteral("Connection error");
    qDebug() << "Socket error:" << msg;
    emit errorOccurred(msg);
}

void BluetoothService::onDataReady()
{
    if (!m_socket)
        return;

    QByteArray data = m_socket->readAll();
    qDebug() << "Received:" << data.toHex();
    emit dataReceived(QString::fromUtf8(data));
}

void BluetoothService::startReconnectTimer()
{
    if (!m_deviceAddress.isEmpty()) {
        if (!m_reconnectTimer) {
            m_reconnectTimer = new QTimer(this);
            connect(m_reconnectTimer, &QTimer::timeout,
                    this, &BluetoothService::reconnectTimerTick);
        }
        m_reconnectTimer->start(3000);
    }
}

void BluetoothService::reconnectTimerTick()
{
    if (m_socket && m_socket->state() != QBluetoothSocket::ConnectedState) {
        qDebug() << "Attempting reconnection to" << m_deviceAddress;
        m_socket->connectToService(QBluetoothAddress(m_deviceAddress), SPP_UUID, QIODevice::ReadWrite);
    }
}
