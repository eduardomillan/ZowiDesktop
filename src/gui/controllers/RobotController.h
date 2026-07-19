#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <zowi/bluetooth_api.h>
#include <zowi/stk500v1.h>

class SessionController;

// Robot connection controller. Despite its historical name it is transport
// agnostic: it can talk to the robot either over Bluetooth SPP (the Qt/BlueZ
// backend) or over a USB/serial TTY (the serial backend). The active transport
// is selected automatically (USB preferred when a Zowi is detected on a port)
// but the user can override it from the UI. See docs/project for the design.
class RobotController : public QObject
{
    Q_OBJECT
public:
    // Keep the values stable: they are persisted to config/session.
    enum Transport { Auto = 0, Bluetooth = 1, Usb = 2 };
    Q_ENUM(Transport)

    // High-level connection situation derived from the transport state machine
    // (see .local/transport_thoughts.md). The UI reacts to this instead of
    // letting the user pick a transport manually.
    enum Situation {
        Demo = 0,          // no transport available: demo mode
        Unregistered = 1,  // transport(s) available, no Zowi registered yet
        Connecting = 2,    // attempting to (re)connect to the registered Zowi
        Connected = 3,     // registered Zowi connected
        Disconnected = 4,  // registered, transport available, not connected
        TransportLost = 5  // registered but its transport is unavailable now
    };
    Q_ENUM(Situation)

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
    // Firmware / program id reported by the robot (e.g. "&&I <appId>%%"). Empty
    // until received. Persisted to session as activeZowiAppId so the app knows
    // which firmware the connected Zowi is running.
    Q_PROPERTY(QString appId READ appId NOTIFY appIdChanged)
    Q_PROPERTY(int battery READ battery NOTIFY batteryChanged)
    Q_PROPERTY(bool restoring READ isRestoring NOTIFY restoringChanged)
    // Situation enum values surfaced to QML (context-property objects can't
    // reference Q_ENUM members directly).
    Q_PROPERTY(int SituationDemo READ situationDemo CONSTANT)
    Q_PROPERTY(int SituationUnregistered READ situationUnregistered CONSTANT)
    Q_PROPERTY(int SituationConnecting READ situationConnecting CONSTANT)
    Q_PROPERTY(int SituationConnected READ situationConnected CONSTANT)
    Q_PROPERTY(int SituationDisconnected READ situationDisconnected CONSTANT)
    Q_PROPERTY(int SituationTransportLost READ situationTransportLost CONSTANT)
    Q_PROPERTY(int situation READ situation NOTIFY situationChanged)

public:
    explicit RobotController(QObject *parent = nullptr);

    int transportAuto() const { return Auto; }
    int transportBluetooth() const { return Bluetooth; }
    int transportUsb() const { return Usb; }

    int situationDemo() const { return Demo; }
    int situationUnregistered() const { return Unregistered; }
    int situationConnecting() const { return Connecting; }
    int situationConnected() const { return Connected; }
    int situationDisconnected() const { return Disconnected; }
    int situationTransportLost() const { return TransportLost; }
    int situation() const;

    bool isBluetoothAvailable() const;
    bool isUsbAvailable() const;
    int transport() const;
    int activeTransport() const;
    bool isConnected() const;
    bool isConnecting() const;
    bool isScanning() const;
    QString deviceName() const;
    QString deviceAddress() const;
    QString appId() const;
    int battery() const;
    bool isRestoring() const { return m_restoring; }

    void setTransport(int transport);
    Q_INVOKABLE bool switchTransport(int transport);
    Q_INVOKABLE void setTransportPreference(int transport);

    Q_INVOKABLE void setDeviceName(const QString &name);

    // Transport / USB helpers.
    Q_INVOKABLE QStringList listUsbPorts() const;
    Q_INVOKABLE void refreshTransports();
    Q_INVOKABLE void connectUsb(const QString &port = QString());
    Q_INVOKABLE void restoreFirmware(const QString &firmwarePath);

    // Share the session controller so transport persistence goes through the
    // same store the UI reads (and that emits sessionChanged for the DEV view).
    void setSessionController(SessionController *session) { m_session = session; }

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void stopScan();
    Q_INVOKABLE void connectToDevice(const QString &address);
    Q_INVOKABLE void disconnectFromDevice();
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void unpairDevice(const QString &address);
    Q_INVOKABLE void sendData(const QString &data);
    Q_INVOKABLE void confirmRestoreBattery(bool proceed);

signals:
    void deviceDiscovered(const QString &name, const QString &address);
    void scanFinished();
    void connectionChanged();
    void connectingChanged();
    void scanningChanged();
    void deviceChanged();
    void batteryChanged();
    void appIdChanged();
    void dataReceived(const QString &data);
    void errorOccurred(const QString &message);
    void firmwareRestoreStarted();
    void firmwareRestoreProgress(int percent, int written, int total);
    void firmwareRestoreFinished(bool success, const QString &message);
    void firmwareRestoreBatteryLow(float level);
    void unpairFinished(bool success, const QString &message);
    void transportChanged();
    void activeTransportChanged();
    void transportsChanged();
    void restoringChanged();
    void situationChanged();

private:
    void parseIncoming();
    void requestRobotData();
    void setConnecting(bool value);

    // Backend management.
    void useBluetoothBackend();
    void useSerialBackend();
    void wireBackend();
    void setActiveTransport(Transport t);
    void revertTransport(Transport prev);

    // Situation state machine helpers. computeSituation() derives the current
    // Situation from availability + registration + connection; maybeEmitSituation()
    // recomputes and emits situationChanged() only when it actually changed.
    Situation computeSituation() const;
    void maybeEmitSituation();
    // Persist the transport the active Zowi was registered with (bt/usb) so the
    // transport stays tied to the registration.
    void persistRegistrationTransport(Transport t);
    Situation m_situation = Demo;

    // Hotplug / detection.
    void pollTransports();
    // Try to identify a real Zowi on `port` with a short I\r handshake. The
    // probe opens the port (which resets the robot via DTR) so it is only run
    // once per newly-seen port while disconnected. Returns the port if a Zowi
    // replied, or an empty string otherwise.
    QString probeZowiOnPort(const QString &port);

    // Firmware restore. The reset/reconnect and the post-upload battery check
    // run on the GUI thread (they touch the backend's QSocketNotifier, which is
    // not thread-safe). Only the blocking STK500 upload itself runs on a
    // dedicated worker thread (runUpload) so the UI stays responsive.
    Q_INVOKABLE void continueAfterUpload(bool ok);
    void proceedWithRestore();
    void finishRestore(bool success);
    void setRestoring(bool value);

    // Firmware upload state (mirrors CLI's g_uploadMode / g_stkBuffer).
    bool m_uploadMode = false;
    std::string m_stkBuffer;
    std::mutex m_uploadMutex;

    // Battery-confirmation handshake (Phase 3): after a successful upload the
    // GUI checks the reported battery; if it is low it asks the user to confirm
    // via the UI and defers finishing until confirmRestoreBattery() is called.
    bool m_batteryPending = false;

    // Restore context carried from restoreFirmware() (GUI thread) to the
    // post-upload continuation.
    QString m_restoreLocalPath;
    QString m_restoreOriginalPath;
    QString m_restoreTarget;
    bool m_restoreIsUsb = false;

    std::unique_ptr<zowi::BluetoothApi> m_backend;
    Transport m_backendKind = Bluetooth;   // which backend is currently built
    Transport m_transport = Auto;          // user/persisted preference
    Transport m_activeTransport = Bluetooth; // transport actually in use

    bool m_connected = false;
    bool m_connecting = false;
    bool m_scanning = false;
    QString m_deviceName;
    QString m_deviceAddress;
    QString m_appId;
    QString m_usbPort;
    float m_battery = -1.0f;
    std::string m_rxBuffer;

    QStringList m_knownUsbPorts;   // ports currently present
    QStringList m_probedUsbPorts;  // ports already handshake-probed this session
    bool m_bluetoothAvailable = false;
    bool m_usbAvailable = false;
    QTimer m_pollTimer;
    QTimer m_dataPollTimer;   // re-requests name/appId/battery while connected
    int m_usbBaud = 9600;
    int m_usbBootloaderBaud = 115200;
    int m_transportTimeoutMs = 1500;
    SessionController *m_session = nullptr;
    bool m_restoring = false;
    bool m_simulateLowBattery = false; // TEMP: force low-battery dialog for testing
    float m_lowBatteryThreshold = 50.0f; // configurable low-battery threshold
};
