#include "zowibluetoothcontroller.h"
#include "translator.h"
#include <QDebug>

static const QBluetoothUuid SPP_UUID = QBluetoothUuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));

ZowiBluetoothController::ZowiBluetoothController(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(nullptr)
    , m_socket(nullptr)
    , m_reconnectTimer(nullptr)
    , m_scanning(false)
{
}

bool ZowiBluetoothController::isConnected() const
{
    return m_socket && m_socket->state() == QBluetoothSocket::ConnectedState;
}

bool ZowiBluetoothController::isScanning() const
{
    return m_scanning;
}

QString ZowiBluetoothController::deviceName() const
{
    return m_deviceName;
}

QString ZowiBluetoothController::deviceAddress() const
{
    return m_deviceAddress;
}

void ZowiBluetoothController::startScan()
{
    if (m_scanning)
        return;

    if (m_discoveryAgent) {
        m_discoveryAgent->deleteLater();
        m_discoveryAgent = nullptr;
    }

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &ZowiBluetoothController::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &ZowiBluetoothController::onScanFinished);
    connect(m_discoveryAgent,
            QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
            this, &ZowiBluetoothController::onScanError);

    m_discoveredDevices.clear();
    m_scanning = true;
    emit scanningChanged();
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
}

void ZowiBluetoothController::stopScan()
{
    if (m_discoveryAgent && m_scanning) {
        m_discoveryAgent->stop();
        m_scanning = false;
        emit scanningChanged();
    }
}

void ZowiBluetoothController::connectToDevice(const QString &address)
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
            this, &ZowiBluetoothController::onSocketConnected);
    connect(m_socket, &QBluetoothSocket::disconnected,
            this, &ZowiBluetoothController::onSocketDisconnected);
    connect(m_socket,
            QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
            this, &ZowiBluetoothController::onSocketError);
    connect(m_socket, &QBluetoothSocket::readyRead,
            this, &ZowiBluetoothController::onDataReady);

    qDebug() << "Connecting to" << address << "via RFCOMM SPP...";
    m_socket->connectToService(QBluetoothAddress(address), SPP_UUID, QIODevice::ReadWrite);
}

void ZowiBluetoothController::disconnectFromDevice()
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

void ZowiBluetoothController::sendData(const QString &data)
{
    if (!m_socket || m_socket->state() != QBluetoothSocket::ConnectedState) {
        emit errorOccurred(Translator::tr("ZowiBluetoothController", "Not connected to any device"));
        return;
    }

    QByteArray bytes = data.toUtf8();
    qDebug() << "Sending:" << bytes.toHex();
    m_socket->write(bytes);
}

void ZowiBluetoothController::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
        return;

    QString name = device.name().trimmed();
    if (name.isEmpty())
        name = "(unknown)";

    m_discoveredDevices.insert(device.address().toString(), name);

    qDebug() << "Discovered device:" << name << device.address().toString();
    emit deviceDiscovered(name, device.address().toString());
}

void ZowiBluetoothController::onScanFinished()
{
    m_scanning = false;
    emit scanningChanged();
    emit scanFinished();
}

void ZowiBluetoothController::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error)
    m_scanning = false;
    emit scanningChanged();
    QString msg = m_discoveryAgent ? m_discoveryAgent->errorString() : Translator::tr("ZowiBluetoothController", "Scan error");
    emit errorOccurred(msg);
}

void ZowiBluetoothController::onSocketConnected()
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

void ZowiBluetoothController::onSocketDisconnected()
{
    qDebug() << "Disconnected from" << m_deviceAddress;
    emit connectionChanged();
    startReconnectTimer();
}

void ZowiBluetoothController::onSocketError(QBluetoothSocket::SocketError error)
{
    Q_UNUSED(error)
    QString msg = m_socket ? m_socket->errorString() : Translator::tr("ZowiBluetoothController", "Connection error");
    qDebug() << "Socket error:" << msg;
    emit errorOccurred(msg);
}

void ZowiBluetoothController::onDataReady()
{
    if (!m_socket)
        return;

    QByteArray data = m_socket->readAll();
    qDebug() << "Received:" << data.toHex();
    emit dataReceived(QString::fromUtf8(data));
}

void ZowiBluetoothController::startReconnectTimer()
{
    if (!m_deviceAddress.isEmpty()) {
        if (!m_reconnectTimer) {
            m_reconnectTimer = new QTimer(this);
            connect(m_reconnectTimer, &QTimer::timeout,
                    this, &ZowiBluetoothController::reconnectTimerTick);
        }
        m_reconnectTimer->start(3000);
    }
}

void ZowiBluetoothController::reconnectTimerTick()
{
    if (m_socket && m_socket->state() != QBluetoothSocket::ConnectedState) {
        qDebug() << "Attempting reconnection to" << m_deviceAddress;
        m_socket->connectToService(QBluetoothAddress(m_deviceAddress), SPP_UUID, QIODevice::ReadWrite);
    }
}
