#include "qt_bluetooth_backend.h"
#include <iostream>
#include <QDebug>
#include <QCoreApplication>
#include <chrono>
#include <thread>
#include <QBluetoothAddress>
#include <QBluetoothUuid>
#include <QBluetoothServiceInfo>
#include <QDBusReply>

static const QBluetoothUuid SPP_UUID = QBluetoothUuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));
static const QString AGENT_DBUS_PATH = QStringLiteral("/edu/um/ZowiDesktop/agent");

namespace zowi {

QtBluetoothBackend::QtBluetoothBackend(QObject *parent)
    : QObject(parent)
{
    QObject::connect(&m_localDevice, &QBluetoothLocalDevice::pairingFinished,
            this, [this](const QBluetoothAddress &, QBluetoothLocalDevice::Pairing pairing) {
        if (m_onUnpairResult) {
            auto cb = std::move(m_onUnpairResult);
            cb(pairing == QBluetoothLocalDevice::Unpaired, {});
        }
    });
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(&m_localDevice, &QBluetoothLocalDevice::errorOccurred,
#else
    QObject::connect(&m_localDevice,
            QOverload<QBluetoothLocalDevice::Error>::of(&QBluetoothLocalDevice::error),
#endif
            this, [this](QBluetoothLocalDevice::Error) {
        if (m_onUnpairResult) {
            auto cb = std::move(m_onUnpairResult);
            cb(false, "failed to unpair device");
        }
    });
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

// ── BlueZ agent for automatic PIN entry ────────────────────────
//
// The HC-05 modules used by Zowi robots require legacy pairing with
// a fixed PIN ("1234").  Qt's Bluetooth stack shows a GUI dialog for
// this, which is useless in a CLI context.  We register our own agent
// on D-Bus so that BlueZ calls us when it needs the PIN, and we reply
// immediately with "1234".

void QtBluetoothBackend::registerBlueZAgent()
{
    if (m_agent)
        return;

    auto bus = QDBusConnection::systemBus();
    m_agentPath = AGENT_DBUS_PATH;

    m_agent.reset(new BlueZAgent(QStringLiteral("1234"), this));
    if (!bus.registerObject(m_agentPath, m_agent.get(), QDBusConnection::ExportAllSlots)) {
        m_agent.reset();
        return;
    }

    QDBusInterface agentMgr(QStringLiteral("org.bluez"), QStringLiteral("/org/bluez"),
                            QStringLiteral("org.bluez.AgentManager1"), bus);
    QDBusReply<void> reply = agentMgr.call(QStringLiteral("RegisterAgent"),
        QVariant::fromValue(QDBusObjectPath(m_agentPath)),
        QStringLiteral("KeyboardOnly"));

    if (!reply.isValid()) {
        bus.unregisterObject(m_agentPath);
        m_agent.reset();
        return;
    }

    // Make our agent the default so BlueZ routes PIN requests to it
    // instead of showing a GUI dialog (which is useless in CLI mode).
    agentMgr.call(QStringLiteral("RequestDefaultAgent"),
                  QVariant::fromValue(QDBusObjectPath(m_agentPath)));
}

void QtBluetoothBackend::unregisterBlueZAgent()
{
    if (!m_agent)
        return;

    auto bus = QDBusConnection::systemBus();
    QDBusInterface agentMgr(QStringLiteral("org.bluez"), QStringLiteral("/org/bluez"),
                            QStringLiteral("org.bluez.AgentManager1"), bus);
    agentMgr.call(QStringLiteral("UnregisterAgent"),
                  QVariant::fromValue(QDBusObjectPath(m_agentPath)));
    bus.unregisterObject(m_agentPath);
    m_agent.reset();
    m_agentPath.clear();
}

// ── Connection ─────────────────────────────────────────────────

bool QtBluetoothBackend::connect(const std::string &address)
{
    m_deviceAddress = QString::fromStdString(address);
    m_deviceName = m_discoveredDevices.value(m_deviceAddress);

    if (m_socket) {
        m_socket->disconnectFromService();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    registerBlueZAgent();

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

        if (QCoreApplication::instance()) {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
            while (m_socket->state() != QBluetoothSocket::SocketState::UnconnectedState &&
                   std::chrono::steady_clock::now() < deadline)
            {
                QCoreApplication::processEvents(QEventLoop::AllEvents);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        m_socket->deleteLater();
        m_socket = nullptr;
    }

    unregisterBlueZAgent();

    if (m_reconnectTimer) {
        m_reconnectTimer->deleteLater();
        m_reconnectTimer = nullptr;
    }
}

bool QtBluetoothBackend::send(const std::string &data)
{
    if (!m_socket || m_socket->state() != QBluetoothSocket::SocketState::ConnectedState) {
        m_pendingWrite += data;
        return true;
    }

    QByteArray bytes = QByteArray::fromStdString(data);
    m_socket->write(bytes);
    m_socket->waitForBytesWritten(5000);
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

void QtBluetoothBackend::setAutoReconnect(bool enabled, int reconnectIntervalMs)
{
    m_autoReconnect = enabled;
    m_reconnectInterval = reconnectIntervalMs;
    if (!enabled && m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
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

    if (!m_pendingWrite.empty()) {
        QByteArray bytes = QByteArray::fromStdString(m_pendingWrite);
        m_socket->write(bytes);
        m_socket->waitForBytesWritten(5000);
        m_pendingWrite.clear();
    }

    if (m_onConnect) m_onConnect(true);
}

void QtBluetoothBackend::onSocketDisconnected()
{
    if (m_onConnect) m_onConnect(false);
    if (m_autoReconnect) startReconnectTimer();
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
        m_reconnectTimer->start(m_reconnectInterval);
    }
}

void QtBluetoothBackend::reconnectTimerTick()
{
    if (m_deviceAddress.isEmpty())
        return;
    if (m_socket && m_socket->state() == QBluetoothSocket::SocketState::ConnectedState)
        return;

    if (m_socket) {
        m_socket->disconnectFromService();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    registerBlueZAgent();

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
}

void QtBluetoothBackend::unpair(const std::string &address)
{
    m_localDevice.requestPairing(
        QBluetoothAddress(QString::fromStdString(address)),
        QBluetoothLocalDevice::Unpaired);
}

} // namespace zowi
