#pragma once

#include <zowi/bluetooth_api.h>
#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QTimer>
#include <QHash>
#include <QVector>

namespace zowi {

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
};

} // namespace zowi
