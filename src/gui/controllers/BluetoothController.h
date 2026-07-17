#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <memory>
#include <string>
#include <zowi/bluetooth_api.h>

// Robot connection controller. Despite its historical name it is transport
// agnostic: it can talk to the robot either over Bluetooth SPP (the Qt/BlueZ
// backend) or over a USB/serial TTY (the serial backend). The active transport
// is selected automatically (USB preferred when a Zowi is detected on a port)
// but the user can override it from the UI. See docs/project for the design.
class BluetoothController : public QObject
{
    Q_OBJECT
public:
    // Keep the values stable: they are persisted to config/session.
    enum Transport { Auto = 0, Bluetooth = 1, Usb = 2 };
    Q_ENUM(Transport)

private:
    // Enum values exposed to QML (context-property objects can't reference
    // Q_ENUM members directly, so surface them as constants).
    Q_PROPERTY(int TransportAuto READ transportAuto CONSTANT)
    Q_PROPERTY(int TransportBluetooth READ transportBluetooth CONSTANT)
    Q_PROPERTY(int TransportUsb READ transportUsb CONSTANT)
    Q_PROPERTY(bool bluetoothAvailable READ isBluetoothAvailable NOTIFY transportsChanged)
    Q_PROPERTY(bool usbAvailable READ isUsbAvailable NOTIFY transportsChanged)
    Q_PROPERTY(int transport READ transport WRITE setTransport NOTIFY transportChanged)
    Q_PROPERTY(int activeTransport READ activeTransport NOTIFY activeTransportChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(bool connecting READ isConnecting NOTIFY connectingChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceAddress READ deviceAddress NOTIFY deviceChanged)
    Q_PROPERTY(int battery READ battery NOTIFY batteryChanged)

public:
    explicit BluetoothController(QObject *parent = nullptr);

    int transportAuto() const { return Auto; }
    int transportBluetooth() const { return Bluetooth; }
    int transportUsb() const { return Usb; }

    bool isBluetoothAvailable() const;
    bool isUsbAvailable() const;
    int transport() const;
    int activeTransport() const;
    bool isConnected() const;
    bool isConnecting() const;
    bool isScanning() const;
    QString deviceName() const;
    QString deviceAddress() const;
    int battery() const;

    void setTransport(int transport);

    Q_INVOKABLE void setDeviceName(const QString &name);

    // Transport / USB helpers.
    Q_INVOKABLE QStringList listUsbPorts() const;
    Q_INVOKABLE void refreshTransports();
    Q_INVOKABLE void connectUsb(const QString &port = QString());

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void stopScan();
    Q_INVOKABLE void connectToDevice(const QString &address);
    Q_INVOKABLE void disconnectFromDevice();
    Q_INVOKABLE void unpairDevice(const QString &address);
    Q_INVOKABLE void sendData(const QString &data);

signals:
    void deviceDiscovered(const QString &name, const QString &address);
    void scanFinished();
    void connectionChanged();
    void connectingChanged();
    void scanningChanged();
    void deviceChanged();
    void batteryChanged();
    void dataReceived(const QString &data);
    void errorOccurred(const QString &message);
    void unpairFinished(bool success, const QString &message);
    void transportChanged();
    void activeTransportChanged();
    void transportsChanged();

private:
    void parseIncoming();
    void setConnecting(bool value);

    // Backend management.
    void useBluetoothBackend();
    void useSerialBackend();
    void wireBackend();
    void setActiveTransport(Transport t);

    // Hotplug / detection.
    void pollTransports();
    // Try to identify a real Zowi on `port` with a short I\r handshake. The
    // probe opens the port (which resets the robot via DTR) so it is only run
    // once per newly-seen port while disconnected. Returns the port if a Zowi
    // replied, or an empty string otherwise.
    QString probeZowiOnPort(const QString &port);

    std::unique_ptr<zowi::BluetoothApi> m_backend;
    Transport m_backendKind = Bluetooth;   // which backend is currently built
    Transport m_transport = Auto;          // user/persisted preference
    Transport m_activeTransport = Bluetooth; // transport actually in use

    bool m_connected = false;
    bool m_connecting = false;
    bool m_scanning = false;
    QString m_deviceName;
    QString m_deviceAddress;
    QString m_usbPort;
    float m_battery = -1.0f;
    std::string m_rxBuffer;

    QStringList m_knownUsbPorts;   // ports currently present
    QStringList m_probedUsbPorts;  // ports already handshake-probed this session
    bool m_bluetoothAvailable = false;
    bool m_usbAvailable = false;
    QTimer m_pollTimer;
    int m_usbBaud = 9600;
};
