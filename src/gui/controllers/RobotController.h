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
#include <atomic>
#include <zowi/bluetooth_api.h>
#include <zowi/message_parser.h>
#include <zowi/robot_state.h>
#include <zowi/firmware_installer.h>

class SessionController;

// State of the in-flight USB identification probe (see startUsbProbe).
// Defined in the .cpp so the backend alias stays private.
struct UsbProbeSession;

// Robot connection controller. Despite its historical name it is transport
// agnostic: it can talk to the robot either over Bluetooth SPP (the Qt/BlueZ
// backend) or over a USB/serial TTY (the serial backend). The active transport
// is selected automatically (Bluetooth preferred when available; USB is only
// used as fallback when no Bluetooth adapter is present). Once a Zowi is
// registered, its registered transport (`activeZowiTransport`) is honoured —
// changing it requires forgetting the Zowi. See docs/project/TRANSPORT_HOWTO.md
// for the design.

// Result carrier for the background Bluetooth-adapter probe (see
// RobotController::pollTransports). Shared with the worker thread so a probe
// that is still running when the controller is destroyed can finish safely
// without touching a destroyed instance.
struct BtCheckState {
    std::atomic<bool> result{false};
    std::atomic<bool> done{false};
};

class RobotController : public QObject
{
    Q_OBJECT
public:
    // Keep the values stable: they are persisted to config/session.
    enum Transport { Auto = 0, Bluetooth = 1, Usb = 2 };
    Q_ENUM(Transport)

    // High-level connection situation derived from the transport state machine
    // (see docs/project/TRANSPORT_HOWTO.md). The UI reacts to this instead of
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
    // True when a Zowi is registered to USB and its registered port is present
    // right now (as opposed to usbAvailable, which is true for any serial port
    // even before it is confirmed to be a Zowi).
    Q_PROPERTY(bool usbZowiConfirmed READ isUsbZowiConfirmed NOTIFY transportsChanged)
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
    ~RobotController();

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
    bool isUsbZowiConfirmed() const;
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

    // Pause/resume the periodic data poll (name/appId/battery). Used while the
    // calibration screen is open so the BLE link carries only live G commands.
    Q_INVOKABLE void setDataPollingEnabled(bool enabled);

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
    // Emitted when the robot sends &&A%% (command accepted, not yet processed),
    // e.g. right after a movement command starts being handled.
    void softwareAckReceived();
    // Emitted when the robot sends &&F%% (command fully processed), e.g. after
    // a melody finishes playing.
    void finalAckReceived();
    void errorOccurred(const QString &message);
    void usbIdentityMismatch();
    void firmwareRestoreStarted();
    void firmwareRestoreProgress(int percent, int written, int total);
    void firmwareRestoreFinished(bool success, const QString &message);
    void firmwareRestoreBatteryLow(float level);
    void unpairFinished(bool success, const QString &message);
    void transportChanged();
    void activeTransportChanged();
    void transportsChanged();
    void bothTransportsAvailable();
    void restoringChanged();
    void situationChanged();

private:
    void parseIncoming();
    void requestRobotData();
    void setConnecting(bool value);
    // Fired when a connection attempt outlives connect_timeout (config.json):
    // cancels the attempt and drops the situation into Demo instead of hanging
    // on "Connecting..." forever.
    void onConnectTimeout();

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
    // Picks the backend from availability + registration + (legacy) preference.
    // Called by refreshTransports() and re-run whenever availability changes
    // while idle, so the connection is never left on a non-registered
    // transport once a transport appears/disappears.
    void applyTransportSelection();
    // Non-blocking USB identification probe (see connectUsb). When no port is
    // known, the candidate ports are probed one by one on the GUI thread,
    // driven by m_usbProbeTimer: the serial backend's QSocketNotifier delivers
    // bytes through the event loop, so probing never blocks the GUI. It used
    // to spin synchronously for up to kProbeTimeoutMs per port, which froze
    // the splash→home transition for USB-registered devices.
    void startUsbProbe(const QStringList &ports);
    void probeNextUsbPort();
    void usbProbeTick();
    // The port is taken by value: it may alias the in-flight probe session,
    // which the implementation destroys before the port is used.
    void finishUsbProbe(QString port);

    // Firmware restore. The reset/reconnect and the post-upload battery check
    // run on the GUI thread (they touch the backend's QSocketNotifier, which is
    // not thread-safe). Only the blocking STK500 upload itself runs on a
    // dedicated worker thread (runUpload) so the UI stays responsive.
    Q_INVOKABLE void continueAfterUpload(bool ok);
    void proceedWithRestore();
    void finishRestore(bool success);
    void setRestoring(bool value);

    // Firmware upload orchestrator (shared with CLI and future hosts).
    zowi::FirmwareInstaller m_firmwareInstaller;

    // Battery-confirmation handshake (Phase 3): after a successful upload the
    // GUI checks the reported battery; if it is low it asks the user to confirm
    // via the UI and defers finishing until confirmRestoreBattery() is called.
    bool m_batteryPending = false;

    bool m_verifyPending = false;
    QString m_verifyExpectedName;

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
    zowi::MessageParser m_parser;  // robot → host frame reassembly (see docs/project/ZOWILIBS.md)
    zowi::RobotState m_robotState; // mirror of the robot's name/appId/battery

    QStringList m_knownUsbPorts;   // ports currently present
    // In-flight USB identification probe (see startUsbProbe). Owned as a
    // unique_ptr so tearing the controller down stops the timer and closes the
    // probe port before the backends are destroyed.
    std::unique_ptr<UsbProbeSession> m_usbProbe;
    bool m_bluetoothAvailable = false;
    bool m_usbAvailable = false;
    QTimer m_pollTimer;
    QTimer m_dataPollTimer;   // re-requests name/appId/battery while connected
    int m_usbBaud = 9600;
    int m_usbBootloaderBaud = 115200;
    int m_transportTimeoutMs = 1500;
    // Max time (ms) for any connection attempt before falling back to Demo.
    QTimer m_connectTimer;             // single-shot, armed by setConnecting(true)
    bool m_connectTimedOut = false;    // last attempt timed out: pin situation to Demo
    QTimer m_usbProbeTimer;            // repeating: drives the USB identity probe
    int m_connectTimeoutMs = 10000;
    // True while a background demo-mode retry is in flight (no UI state).
    bool m_silentRetryPending = false;
    SessionController *m_session = nullptr;
    bool m_restoring = false;
    bool m_simulateLowBattery = false; // TEMP: force low-battery dialog for testing
    float m_lowBatteryThreshold = 50.0f; // configurable low-battery threshold

    // The Bluetooth adapter is probed on a detached background thread: on
    // machines with no Bluetooth hardware the platform query can block for
    // many seconds, so the GUI thread must never call it synchronously. The
    // worker owns the state (shared_ptr), so a probe still running at teardown
    // can finish without touching a destroyed controller.
    std::shared_ptr<BtCheckState> m_btCheckState;
    bool m_btAvailLast = false;   // last completed probe result
};
