#pragma once

#include <zowi/bluetooth_api.h>
#include <QObject>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QTimer>
#include <QHash>
#include <QVector>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusInterface>
#include <memory>

namespace zowi {

// BlueZ D-Bus agent that responds to PIN requests — required because HC-05
// modules need pairing with pin "1234" before they accept SPP connections.
class BlueZAgent : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.bluez.Agent1")
public:
    explicit BlueZAgent(const QString &pin, QObject *parent = nullptr)
        : QObject(parent), m_pin(pin) {}

public slots:
    QString RequestPinCode(const QDBusObjectPath &device) {
        Q_UNUSED(device)
        return m_pin;
    }
    void DisplayPinCode(const QDBusObjectPath &device, const QString &pincode) {
        Q_UNUSED(device) Q_UNUSED(pincode)
    }
    void RequestConfirmation(const QDBusObjectPath &device, const QString &passkey) {
        Q_UNUSED(device) Q_UNUSED(passkey)
    }
    void AuthorizeService(const QDBusObjectPath &device, const QString &uuid) {
        Q_UNUSED(device) Q_UNUSED(uuid)
    }
    void Cancel() {}

private:
    QString m_pin;
};

class QtBluetoothBackend : public QObject, public BluetoothApi {
    Q_OBJECT
public:
    explicit QtBluetoothBackend(QObject *parent = nullptr);
    ~QtBluetoothBackend() override;

    // BluetoothApi
    bool init() override;
    void startDiscovery() override;
    void stopDiscovery() override;
    bool connect(const std::string &address) override;
    void disconnect() override;
    bool send(const std::string &data) override;
    bool isConnected() const override;
    std::string lastError() const override;
    void setAutoReconnect(bool enabled, int reconnectIntervalMs = 3000) override;
    void unpair(const std::string &address) override;

    // Extra Qt-specific getters for the bridge layer
    QString deviceName() const;
    QString deviceAddress() const;
    bool isScanning() const;

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onScanFinished();
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QBluetoothSocket::SocketError error);
    void onDataReady();
    void reconnectTimerTick();

private:
    void startReconnectTimer();
    void registerBlueZAgent();
    void unregisterBlueZAgent();

    QBluetoothLocalDevice m_localDevice;
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QBluetoothSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QHash<QString, QString> m_discoveredDevices;
    bool m_scanning = false;
    QString m_deviceName;
    QString m_deviceAddress;
    std::string m_lastError;
    std::string m_pendingWrite;
    bool m_autoReconnect = true;
    int m_reconnectInterval = 3000;
    std::unique_ptr<BlueZAgent> m_agent;
    QString m_agentPath;
};

} // namespace zowi
