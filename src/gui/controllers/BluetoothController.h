#pragma once

#include <QObject>
#include <memory>
#include <zowi/bluetooth_api.h>

class BluetoothController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceAddress READ deviceAddress NOTIFY deviceChanged)

public:
    explicit BluetoothController(QObject *parent = nullptr);

    bool isConnected() const;
    bool isScanning() const;
    QString deviceName() const;
    QString deviceAddress() const;

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void stopScan();
    Q_INVOKABLE void connectToDevice(const QString &address);
    Q_INVOKABLE void disconnectFromDevice();
    Q_INVOKABLE void sendData(const QString &data);

signals:
    void deviceDiscovered(const QString &name, const QString &address);
    void scanFinished();
    void connectionChanged();
    void scanningChanged();
    void deviceChanged();
    void dataReceived(const QString &data);
    void errorOccurred(const QString &message);

private:
    std::unique_ptr<zowi::BluetoothApi> m_backend;
    bool m_connected = false;
    bool m_scanning = false;
    QString m_deviceName;
    QString m_deviceAddress;
};
