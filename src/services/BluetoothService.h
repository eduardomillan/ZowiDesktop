#ifndef BLUETOOTHSERVICE_H
#define BLUETOOTHSERVICE_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QBluetoothDeviceInfo>
#include <QTimer>
#include <QHash>
#include <QVector>
#include "core/DeviceInfo.h"

#ifdef Q_OS_WIN
class BluetoothNativeLoader;
#endif

class BluetoothService : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothService(QObject *parent = nullptr);
    ~BluetoothService();

    bool isConnected() const;
    bool isScanning() const;
    QString deviceName() const;
    QString deviceAddress() const;
    QVector<DeviceInfo> discoveredDevices() const;
    bool isNativeAvailable() const;

    void startScan();
    void stopScan();
    void connectToDevice(const QString &address);
    void disconnectFromDevice();
    void sendData(const QString &data);

signals:
    void deviceDiscovered(const DeviceInfo &device);
    void scanFinished();
    void connectionChanged();
    void scanningChanged();
    void deviceChanged();
    void dataReceived(const QString &data);
    void errorOccurred(const QString &message);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onScanFinished();
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QBluetoothSocket::SocketError error);
    void onDataReady();
    void reconnectTimerTick();

#ifdef Q_OS_WIN
    void onNativeConnected();
    void onNativeDisconnected();
    void onNativeDataReceived(const QByteArray &data);
#endif

private:
    void startReconnectTimer();

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QBluetoothSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;

    QHash<QString, QString> m_discoveredDevices;
    QVector<DeviceInfo> m_deviceList;

    bool m_scanning = false;
    QString m_deviceName;
    QString m_deviceAddress;

#ifdef Q_OS_WIN
    BluetoothNativeLoader *m_nativeLoader = nullptr;
    bool m_useNative = false;
#endif
};

#endif
