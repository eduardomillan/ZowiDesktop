#include "RobotController.h"
#include "SessionController.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>

#ifndef _WIN32
#include "qt_bluetooth_backend.h"
#endif
#ifdef ZOWI_HAVE_SERIAL
#ifdef _WIN32
#include "win_serial_backend.h"
#else
#include "serial_bluetooth_backend.h"
#endif
#endif
#ifdef ZOWI_HAVE_NATIVE_BT
#include "native_bluetooth_backend.h"
#endif
#include <zowi/config_store.h>
#include <zowi/session_store.h>
#include <zowi/transport_constants.h>
#include <zowi/protocol.h>
#include <zowi/stk500v1.h>

#ifdef ZOWI_HAVE_SERIAL
#ifdef _WIN32
using SerialBackend = zowi::WinSerialBackend;
#else
using SerialBackend = zowi::SerialBluetoothBackend;
#endif
#endif

namespace {
// How often (ms) to poll for USB port / Bluetooth adapter appearance.
constexpr int kPollIntervalMs = 2500;
// How long (ms) to wait for a Zowi to answer the identification handshake.
constexpr int kProbeTimeoutMs = 6000;
// Max time (ms) to wait at startup for the first Bluetooth-adapter probe
// (runs before the window is shown; the cap covers hung platform queries).
constexpr int kBtProbeWaitMs = 1000;

RobotController::Transport transportFromString(const QString &s)
{
    const QString v = s.trimmed().toLower();
    if (v == "usb") return RobotController::Usb;
    if (v == "bluetooth" || v == "bt") return RobotController::Bluetooth;
    return RobotController::Auto;
}
} // namespace

RobotController::RobotController(QObject *parent)
    : QObject(parent)
{
    // The user's transport preference is persisted in the session store; the
    // USB operating baud rate default comes from the bundled config.json.
    zowi::SessionStore session;
    m_transport = transportFromString(
        QString::fromStdString(session.getString("transport", "auto")));

    zowi::ConfigStore cfg;
    QFile cfgFile(":/src/config.json");
    if (cfgFile.open(QIODevice::ReadOnly)) {
        cfg.loadFromString(cfgFile.readAll().toStdString());
        cfgFile.close();
    }
    try {
        std::string b = cfg.get("usb_baud");
        if (!b.empty()) m_usbBaud = std::stoi(b);
    } catch (...) {}
    try {
        std::string b = cfg.get("usb_bootloader_baud");
        if (!b.empty()) m_usbBootloaderBaud = std::stoi(b);
    } catch (...) {}
    try {
        std::string t = cfg.get("transport_timeout");
        if (!t.empty()) m_transportTimeoutMs = std::stoi(t);
    } catch (...) {}
    try {
        std::string t = cfg.get("connect_timeout");
        if (!t.empty()) m_connectTimeoutMs = std::stoi(t);
    } catch (...) {}
    // TEMP (restore battery dialog test): force a low-battery reading so the
    // confirmation dialog appears without waiting for the real battery to drain.
    try {
        std::string s = cfg.get("restore_simulate_low_battery");
        m_simulateLowBattery = (s == "true" || s == "1");
    } catch (...) {}
    try {
        std::string t = cfg.get("restore_low_battery_threshold");
        if (!t.empty()) m_lowBatteryThreshold = std::stof(t);
    } catch (...) {}
    // Start on the Bluetooth backend by default; detection may switch it.
    useBluetoothBackend();

    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &RobotController::pollTransports);
    m_pollTimer.start();

    // Periodically ask the robot for its name, firmware id and battery. The
    // firmware only reports these on request, so keep polling while connected.
    m_dataPollTimer.setInterval(zowi::kIdentityPollMs);
    connect(&m_dataPollTimer, &QTimer::timeout, this, &RobotController::requestRobotData);

    // Connection-attempt watchdog: every setConnecting(true) arms it; if the
    // attempt has not landed within connect_timeout (config.json) the situation
    // falls back to Demo instead of hanging on "Connecting..." forever.
    m_connectTimer.setSingleShot(true);
    m_connectTimer.setInterval(m_connectTimeoutMs);
    connect(&m_connectTimer, &QTimer::timeout, this, &RobotController::onConnectTimeout);

    // Initial availability snapshot + auto-detection.
    refreshTransports();

    // refreshTransports() above started the first Bluetooth probe on a
    // background thread. Give it a short bounded grace period (still before the
    // window is shown, so nothing visibly stalls) so a machine where Bluetooth
    // IS present starts in the correct situation instead of lagging a full poll
    // interval. The cap keeps startup fast even if the platform query hangs.
    if (m_btCheckState) {
        QElapsedTimer btWait;
        btWait.start();
        while (!m_btCheckState->done.load() && btWait.elapsed() < kBtProbeWaitMs) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (m_btCheckState->done.load())
            m_bluetoothAvailable = m_btAvailLast = m_btCheckState->result.load();
    }

    m_situation = computeSituation();
}

RobotController::~RobotController()
{
    m_pollTimer.stop();
    // m_btCheckState is intentionally left untouched: dropping our shared_ptr
    // reference lets a still-running (detached) probe own its state and finish
    // without ever touching a destroyed controller.
}

// --- Backend construction ---------------------------------------------------

void RobotController::useBluetoothBackend()
{
    if (m_backend && m_backendKind == Bluetooth) return;
#ifdef ZOWI_HAVE_NATIVE_BT
    m_backend = std::make_unique<zowi::NativeBluetoothBackend>();
#else
    m_backend = std::make_unique<zowi::QtBluetoothBackend>();
#endif
    m_backendKind = Bluetooth;
    wireBackend();
    setActiveTransport(Bluetooth);
}

void RobotController::useSerialBackend()
{
#ifdef ZOWI_HAVE_SERIAL
    if (m_backend && m_backendKind == Usb) return;
    auto serial = std::make_unique<SerialBackend>();
    serial->setBaudRate(m_usbBaud);
    serial->setBootDelayMs(5000);
    m_backend = std::move(serial);
    m_backendKind = Usb;
    wireBackend();
    setActiveTransport(Usb);
#else
    useBluetoothBackend();
#endif
}

void RobotController::wireBackend()
{
    if (!m_backend) return;

    // A freshly built backend starts with clean protocol framing state: any
    // partial frame left over from the previous transport must not leak into
    // the new connection's stream.
    m_parser.reset();

    m_backend->onDeviceFound([this](const zowi::DeviceInfo &info) {
        auto name  = QString::fromStdString(info.name);
        auto addr  = QString::fromStdString(info.address);
        QMetaObject::invokeMethod(this, [this, name, addr]() {
            emit deviceDiscovered(name, addr);
        }, Qt::QueuedConnection);
    });

    m_backend->onConnectionChanged([this](bool connected) {
        QMetaObject::invokeMethod(this, [this, connected]() {
            m_silentRetryPending = false;
            m_connected = connected;
            emit connectionChanged();
            qInfo() << "[conn] connected=" << connected << "kind=" << m_backendKind
                    << "usbPort=" << m_usbPort << "deviceAddress=" << m_deviceAddress
                    << "known=" << m_knownUsbPorts;
            if (connected) {
                setConnecting(false);
                // A landed attempt clears the demo-after-timeout pin.
                m_connectTimedOut = false;
                if (m_backendKind == Usb && m_deviceAddress.isEmpty()) {
                    if (!m_usbPort.isEmpty())
                        m_deviceAddress = m_usbPort;
                    else if (!m_knownUsbPorts.isEmpty())
                        m_deviceAddress = m_knownUsbPorts.value(0);
                    if (!m_deviceAddress.isEmpty())
                        emit deviceChanged();
                }
                // For Bluetooth, restore m_deviceAddress from the registered Zowi
                // session so it survives the firmware-restore reconnect cycle.
                if (m_backendKind == Bluetooth && m_deviceAddress.isEmpty()) {
                    zowi::SessionStore session;
                    const QString addr = QString::fromStdString(
                        session.getString("activeZowiDeviceAddress"));
                    if (!addr.isEmpty()) {
                        m_deviceAddress = addr;
                        emit deviceChanged();
                    }
                }
                {
                    zowi::SessionStore session;
                    const QString addr = QString::fromStdString(
                        session.getString("activeZowiDeviceAddress"));
                    if (!addr.isEmpty() && m_backendKind == Usb) {
                        m_verifyPending = true;
                        m_verifyExpectedName = QString::fromStdString(
                            session.getString("activeZowiName", "Zowi"));
                    }
                }
                requestRobotData();
                m_dataPollTimer.start();
                {
                    zowi::SessionStore session;
                    const QString addr = QString::fromStdString(
                        session.getString("activeZowiDeviceAddress"));
                    if (!addr.isEmpty() && !m_verifyPending)
                        persistRegistrationTransport(m_backendKind);
                }
            } else {
                setConnecting(false);
                m_dataPollTimer.stop();
                m_verifyPending = false;
                m_verifyExpectedName.clear();
                m_deviceName.clear();
                if (m_backendKind != Usb)
                    m_deviceAddress.clear();
                m_battery = -1.0f;
                if (!m_appId.isEmpty()) {
                    m_appId.clear();
                    emit appIdChanged();
                }
                emit deviceChanged();
                emit batteryChanged();
            }
            maybeEmitSituation();
        }, Qt::QueuedConnection);
    });

    m_backend->onDataReceived([this](const std::string &data) {
        auto qdata = QString::fromStdString(data);
        QMetaObject::invokeMethod(this, [this, qdata]() {
            auto data = qdata.toStdString();
            if (m_firmwareInstaller.isEnabled()) {
                m_firmwareInstaller.feed(data);
            } else {
                m_parser.feed(data);
                parseIncoming();
            }
            qDebug() << "robot rx:" << qdata.trimmed();
            emit dataReceived(qdata);
        }, Qt::QueuedConnection);
    });

    m_backend->onError([this](const std::string &msg) {
        auto qmsg = QString::fromStdString(msg);
        QMetaObject::invokeMethod(this, [this, qmsg]() {
            m_silentRetryPending = false;
            // Silent demo-mode retries probe the saved address in the
            // background; their failures must not spam the UI error paths
            // (MessageBars).
            if (m_connectTimedOut && !m_connecting && !isConnected()) {
                qDebug() << "[conn] silent retry failed:" << qmsg;
                return;
            }
            emit errorOccurred(qmsg);
        }, Qt::QueuedConnection);
    });

    m_backend->onUnpairResult([this](bool ok, const std::string &msg) {
        auto qmsg = QString::fromStdString(msg);
        QMetaObject::invokeMethod(this, [this, ok, qmsg]() {
            emit unpairFinished(ok, qmsg);
        }, Qt::QueuedConnection);
    });

    m_backend->onScanFinished([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            m_scanning = false;
            emit scanningChanged();
            emit scanFinished();
        }, Qt::QueuedConnection);
    });
}

void RobotController::setActiveTransport(Transport t)
{
    if (m_activeTransport == t) return;
    m_activeTransport = t;
    emit activeTransportChanged();
}

// --- Incoming parsing -------------------------------------------------------

void RobotController::parseIncoming()
{
    // The robot frames responses as &&<cmd>[ <value>]%% (e.g. &&B 85.0%%) and
    // can also send legacy line-based messages (e.g. "B 85.0"). Frame
    // reassembly lives in the shared zowi::MessageParser and the identity
    // value rules in zowi::RobotState; this only maps the parsed updates to
    // Qt properties/signals (mirrors the CLI in src/cli/cli_state.cpp).
    bool updated = false;

    auto applyName = [this](const QString &name) {
        if (name != m_deviceName) {
            m_deviceName = name;
            emit deviceChanged();
        }
        if (!m_verifyPending) return false;
        m_verifyPending = false;
        if (m_deviceName == m_verifyExpectedName) {
            if (m_session)
                m_session->saveActiveZowiDeviceAddress(m_deviceAddress);
            persistRegistrationTransport(Usb);
        } else {
            m_verifyExpectedName.clear();
            if (m_backend)
                m_backend->disconnect();
            emit usbIdentityMismatch();
        }
        m_verifyExpectedName.clear();
        return true; // stop processing further messages, as before
    };

    for (const auto &msg : m_parser.drain()) {
        const auto upd = m_robotState.apply(msg);
        if (upd.battery) {
            if (m_robotState.battery != m_battery) {
                m_battery = m_robotState.battery;
                updated = true;
            }
        }
        if (upd.name) {
            if (applyName(QString::fromStdString(m_robotState.name))) break;
        }
        if (upd.appId) {
            QString value = QString::fromStdString(m_robotState.appId);
            if (value != m_appId) {
                m_appId = value;
                emit appIdChanged();
                // Persist the firmware id so the app remembers which firmware
                // this Zowi is running (used later to surface it in the UI).
                if (m_session)
                    m_session->saveActiveZowiAppId(m_appId);
            }
        }
        if (msg.cmd == zowi::toChar(zowi::Command::FinalAck) && !msg.legacy)
            emit finalAckReceived();
    }

    if (updated) emit batteryChanged();
}

void RobotController::requestRobotData()
{
    if (!m_connected || m_firmwareInstaller.isEnabled()) return;
    if (!m_backend) return;
    zowi::sendIdentityQueries(*m_backend);
}

void RobotController::setDataPollingEnabled(bool enabled)
{
    if (enabled) {
        if (m_connected && !m_firmwareInstaller.isEnabled())
            m_dataPollTimer.start(zowi::kIdentityPollMs);
    } else {
        m_dataPollTimer.stop();
    }
}

// --- Property getters -------------------------------------------------------

bool RobotController::isBluetoothAvailable() const
{
    return m_bluetoothAvailable;
}

bool RobotController::isUsbAvailable() const
{
    return m_usbAvailable;
}

bool RobotController::isUsbZowiConfirmed() const
{
    zowi::SessionStore session;
    const QString regAddr = QString::fromStdString(
        session.getString("activeZowiDeviceAddress"));
    if (regAddr.isEmpty()) return false;
    const Transport regT = transportFromString(
        QString::fromStdString(session.getString("activeZowiTransport")));
    if (regT != Usb) return false;
    return m_knownUsbPorts.contains(regAddr);
}

int RobotController::transport() const
{
    return static_cast<int>(m_transport);
}

int RobotController::activeTransport() const
{
    return static_cast<int>(m_activeTransport);
}

bool RobotController::isConnected() const
{
    return m_backend && m_backend->isConnected();
}

bool RobotController::isConnecting() const
{
    return m_connecting;
}

void RobotController::setConnecting(bool value)
{
    if (m_connecting == value) return;
    m_connecting = value;
    if (value)
        m_connectTimer.start(m_connectTimeoutMs);
    else
        m_connectTimer.stop();
    emit connectingChanged();
    maybeEmitSituation();
}

// A connection attempt outlived connect_timeout: cancel it and drop into
// Demo instead of showing "Connecting..." forever. The saved address is kept
// so pollTransports() can keep probing silently in the background; when the
// robot answers, onConnectionChanged(true) resumes the normal flow.
void RobotController::onConnectTimeout()
{
    if (!m_connecting) return;
    qInfo() << "[conn] connect timeout after" << m_connectTimeoutMs
            << "ms -> demo mode";
    m_connectTimedOut = true;
    // Cancel whatever the attempt left behind. Only Bluetooth owns async
    // state here (pending socket, BlueZ agent); the serial backend fails
    // synchronously inside connectUsb(), so there is nothing to clean up.
    if (m_backend && m_backendKind == Bluetooth)
        m_backend->disconnect();
    m_deviceName.clear();
    m_battery = -1.0f;
    emit deviceChanged();
    emit batteryChanged();
    setConnecting(false);
}

bool RobotController::isScanning() const
{
    return m_scanning;
}

QString RobotController::deviceName() const
{
    return m_deviceName;
}

QString RobotController::deviceAddress() const
{
    return m_deviceAddress;
}

QString RobotController::appId() const
{
    return m_appId;
}

int RobotController::battery() const
{
    return m_battery >= 0.0f ? static_cast<int>(std::round(m_battery)) : -1;
}

void RobotController::setDeviceName(const QString &name)
{
    if (m_deviceName != name) {
        m_deviceName = name;
        emit deviceChanged();
    }
}

// --- Transport selection ----------------------------------------------------

void RobotController::setTransport(int transport)
{
    switchTransport(transport);
}

void RobotController::setTransportPreference(int transport)
{
    Transport t = static_cast<Transport>(transport);
    if (t != Auto && t != Bluetooth && t != Usb) return;
    if (m_transport == t) return;
    m_transport = t;
    emit transportChanged();
    if (m_session) {
        const char *s = (t == Usb) ? zowi::kTransportUsb : (t == Bluetooth ? zowi::kTransportBt : "auto");
        m_session->saveString("transport", s);
    }
    // Lightweight switch: update the backend/availability logic without a
    // blocking connect attempt. Used when (re)starting the pairing wizard,
    // where the wizard drives the connection itself.
    refreshTransports();
}

bool RobotController::switchTransport(int transport)
{
    Transport t = static_cast<Transport>(transport);
    if (t != Auto && t != Bluetooth && t != Usb) return false;
    if (m_transport == t) return true;

    const Transport prev = m_transport;

    // Tear down any live link before switching the backend (changing the
    // backend under a live link is a no-op and would leak the old connection).
    if (isConnected()) {
        m_backend->setAutoReconnect(false);
        m_backend->disconnect();
    }

    // Select the target backend; if the chosen transport is unavailable we
    // cannot proceed, so report the error and stay on the current transport.
    if (t == Usb) {
        if (!m_usbAvailable) {
            emit errorOccurred(tr("No USB robot detected"));
            return false;
        }
        useSerialBackend();
    } else if (t == Bluetooth) {
        if (!m_bluetoothAvailable) {
            emit errorOccurred(tr("Bluetooth is not available"));
            return false;
        }
        useBluetoothBackend();
    }

    // Persist the choice so it is honoured next launch. Use the shared session
    // controller (not a throwaway store) so the UI/DEV view sees the update.
    m_transport = t;
    emit transportChanged();
    if (m_session) {
        const char *s = (t == Usb) ? zowi::kTransportUsb : (t == Bluetooth ? zowi::kTransportBt : "auto");
        m_session->saveString("transport", s);
    }

    // Kick off the connection for the chosen transport.
    if (t == Usb) {
        connectUsb();
    } else if (t == Bluetooth) {
        zowi::SessionStore session;
        QString addr = QString::fromStdString(session.getString("activeZowiDeviceAddress"));
        if (addr.isEmpty()) {
            // No known robot: revert and report; Auto would just keep scanning.
            emit errorOccurred(tr("No paired Zowi found to connect over Bluetooth"));
            revertTransport(prev);
            return false;
        }
        connectToDevice(addr);
    } else {
        // Auto: let availability + auto-connect logic pick a transport.
        refreshTransports();
    }

    // Block (pumping the event loop so backend callbacks fire) until connected
    // or the configured timeout elapses. The UI is disabled by the caller while
    // this runs, so the user is effectively "quiet" during the attempt.
    bool ok = false;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < m_transportTimeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        if (isConnected()) { ok = true; break; }
    }

    if (!ok) {
        emit errorOccurred(tr("Could not connect using the selected transport"));
        revertTransport(prev);
        return false;
    }
    return true;
}

void RobotController::revertTransport(Transport prev)
{
    if (isConnected()) {
        m_backend->setAutoReconnect(false);
        m_backend->disconnect();
    }
    if (prev == Usb && m_usbAvailable) useSerialBackend();
    else if (prev == Bluetooth && m_bluetoothAvailable) useBluetoothBackend();
    m_transport = prev;
    emit transportChanged();
    if (m_session) {
        const char *s = (prev == Usb) ? zowi::kTransportUsb : (prev == Bluetooth ? zowi::kTransportBt : "auto");
        m_session->saveString("transport", s);
    }
    if (prev == Usb) connectUsb();
    else if (prev == Bluetooth) refreshTransports();
    else refreshTransports();
}

// --- Situation state machine ------------------------------------------------

int RobotController::situation() const
{
    return static_cast<int>(m_situation);
}

RobotController::Situation RobotController::computeSituation() const
{
    zowi::SessionStore session;
    const QString addr = QString::fromStdString(
        session.getString("activeZowiDeviceAddress"));
    const bool registered = !addr.isEmpty();
    qDebug() << "[situation] addr=" << addr << "registered=" << registered
            << "usb=" << m_usbAvailable << "bt=" << m_bluetoothAvailable
            << "connected=" << isConnected() << "connecting=" << m_connecting
            << "regTransport=" << QString::fromStdString(session.getString("activeZowiTransport"));

    if (!registered) {
        if (m_bluetoothAvailable || m_usbAvailable) return Unregistered;
        return Demo;
    }

    // Registered: the transport is tied to the registration.
    const Transport regT = transportFromString(
        QString::fromStdString(session.getString("activeZowiTransport", "")));
    const bool regTransportAvail =
        (regT == Usb)       ? m_usbAvailable :
        (regT == Bluetooth) ? m_bluetoothAvailable :
        (m_bluetoothAvailable || m_usbAvailable); // legacy: unknown reg transport

    if (isConnected()) return Connected;
    if (m_connecting)  return Connecting;
    if (!regTransportAvail) return TransportLost;
    // The registered transport is there but the robot never answered within
    // connect_timeout: fall back to Demo instead of a permanent "Connecting".
    if (m_connectTimedOut) return Demo;
    return Disconnected;
}

void RobotController::maybeEmitSituation()
{
    Situation next = computeSituation();
    if (next != m_situation) {
        m_situation = next;
        emit situationChanged();
    }
}

void RobotController::persistRegistrationTransport(Transport t)
{
    if (!m_session) return;
    if (t != Bluetooth && t != Usb) return;
    m_session->saveActiveZowiTransport(t == Usb ? zowi::kTransportUsb : zowi::kTransportBt);
    maybeEmitSituation();
}

QStringList RobotController::listUsbPorts() const
{
    QStringList out;
#ifdef ZOWI_HAVE_SERIAL
    for (const auto &p : SerialBackend::listSerialPorts())
        out << QString::fromStdString(p);
#endif
    return out;
}

void RobotController::refreshTransports()
{
    pollTransports();

    // Auto mode: Bluetooth is preferred. USB is only used as fallback when
    // no Bluetooth adapter is present.  However, if a Zowi is already
    // registered, honour its registered transport so we don't switch away
    // from it just because a Bluetooth adapter appeared.
    if (m_transport == Auto && !isConnected() && !m_connecting) {
        zowi::SessionStore session;
        const QString regAddr = QString::fromStdString(
            session.getString("activeZowiDeviceAddress"));
        if (!regAddr.isEmpty()) {
            const Transport regT = transportFromString(
                QString::fromStdString(session.getString("activeZowiTransport", "")));
            if (regT == Usb && m_usbAvailable) {
                if (m_knownUsbPorts.contains(regAddr))
                    m_usbPort = regAddr;
                useSerialBackend();
                return;
            }
            if (regT == Bluetooth && m_bluetoothAvailable) {
                useBluetoothBackend();
                return;
            }
        }
        // No registered Zowi (or its transport unavailable): Auto logic.
        if (m_bluetoothAvailable) {
            useBluetoothBackend();
            return;
        }
        if (m_usbAvailable) {
            // Remember a previously registered USB port so connectUsb() can
            // use it directly. Do NOT open any port here — on Windows the
            // port open asserts DTR and resets the robot, even when we
            // disable DTR immediately after.
            if (!regAddr.isEmpty() && m_knownUsbPorts.contains(regAddr))
                m_usbPort = regAddr;
            useSerialBackend();
            return;
        }
    } else if (m_transport == Usb && m_usbAvailable && !isConnected()) {
        useSerialBackend();
    } else if (m_transport == Bluetooth && !isConnected()) {
        useBluetoothBackend();
    }

    // Notify the UI when both transports are available so it can advise the
    // user to disconnect the USB cable for greater freedom of movement.
    if (m_bluetoothAvailable && m_usbAvailable)
        emit bothTransportsAvailable();
}

void RobotController::pollTransports()
{
    // 1) USB port presence (cheap: does not open the port).
    QStringList ports = listUsbPorts();
    bool usbChanged = (ports != m_knownUsbPorts);
    m_knownUsbPorts = ports;
    // Forget probe results for ports that went away so a re-plug is re-probed.
    for (int i = m_probedUsbPorts.size() - 1; i >= 0; --i)
        if (!ports.contains(m_probedUsbPorts.at(i)))
            m_probedUsbPorts.removeAt(i);

    if (m_backendKind == Usb && m_connected && !m_usbPort.isEmpty()
        && !ports.contains(m_usbPort)) {
        qInfo() << "[conn] USB port vanished while connected, forcing disconnect";
        m_backend->disconnect();
    }

    if (usbChanged && m_backendKind == Usb && !m_connected && !m_connecting 
        && !m_connectTimedOut && !m_usbPort.isEmpty() && ports.contains(m_usbPort)) {
        qInfo() << "[conn] USB port reappeared, auto-reconnecting";
        connectUsb(m_usbPort);
    }

    bool usbAvail = !ports.isEmpty();

    // Bluetooth presence is probed on a detached background thread: the
    // platform query (BlueZ via D-Bus on Linux, WinRT on Windows) can block
    // for many seconds on machines without Bluetooth, so it must never run on
    // the GUI thread. A fresh probe is started only when the previous one has
    // finished; while one is still in flight we keep the last known value so
    // the UI never stalls waiting for the check.
    bool btAvail = m_btAvailLast;
    bool btProbeFinished = m_btCheckState && m_btCheckState->done.load();
    if (btProbeFinished) {
        m_btAvailLast = m_btCheckState->result.load();
        btAvail = m_btAvailLast;
    }
    if (!m_btCheckState || btProbeFinished) {
        auto state = std::make_shared<BtCheckState>();
        m_btCheckState = state;
        std::thread([state]() {
#ifdef ZOWI_HAVE_NATIVE_BT
            state->result = zowi::NativeBluetoothBackend::hasAdapter();
#else
            state->result = zowi::QtBluetoothBackend::hasAdapter();
#endif
            state->done = true;
        }).detach();
    }

    bool changed = usbChanged || (usbAvail != m_usbAvailable) || (btAvail != m_bluetoothAvailable);
    m_usbAvailable = usbAvail;
    m_bluetoothAvailable = btAvail;
    if (changed) {
        qInfo() << "transports:" << "usb=" << usbAvail << "bt=" << btAvail
                << "ports=" << ports;
        emit transportsChanged();
        if (usbAvail && btAvail)
            emit bothTransportsAvailable();
    }

    // Post-timeout recovery. Bluetooth: probe the saved address silently
    // (without flipping the UI back to Connecting); when the robot answers,
    // onConnectionChanged(true) clears the demo pin and resumes normally.
    // USB: the serial backend cannot reconnect on its own, so a newly seen
    // port triggers a fresh (visible) attempt.
    if (m_connectTimedOut && !isConnected() && !m_connecting) {
        if (m_backendKind == Bluetooth && m_bluetoothAvailable
            && !m_deviceAddress.isEmpty() && !m_silentRetryPending) {
            m_silentRetryPending = true;
            qDebug() << "[conn] demo-mode silent retry ->" << m_deviceAddress;
            m_backend->connect(m_deviceAddress.toStdString());
        } else if (m_backendKind == Usb && usbChanged && usbAvail) {
            qDebug() << "[conn] demo-mode USB re-plug -> reconnect";
            connectUsb();
        }
    }

    maybeEmitSituation();
}

QString RobotController::probeZowiOnPort(const QString &port)
{
#ifndef ZOWI_HAVE_SERIAL
    Q_UNUSED(port)
    return QString();
#else
    if (port.isEmpty()) return QString();
    // Only handshake a given port once per session (DTR is disabled so the
    // robot does not reset, but opening/closing is still wasteful to repeat).
    if (m_probedUsbPorts.contains(port)) return QString();
    m_probedUsbPorts << port;

    SerialBackend probe;
    probe.setBaudRate(m_usbBaud);
    // Disable DTR so opening the port does not reset the robot (on Windows
    // the port default asserts DTR, which triggers the Arduino auto-reset).
    // The running firmware stays available and responds to commands right away.
#ifdef _WIN32
    probe.setDtrEnabled(false);
#endif
    probe.setBootDelayMs(0);

    std::string rx;
    bool identified = false;
    probe.onDataReceived([&](const std::string &data) {
        rx += data;
        // Accept the &&I <appId>%% framed reply or the legacy "U " line form.
        if (rx.find("&&I ") != std::string::npos ||
            rx.find("\nU ") != std::string::npos ||
            rx.rfind("U ", 0) == 0) {
            identified = true;
        }
    });

    if (!probe.connect(port.toStdString()))
        return QString();

    // Request the program id. DTR is disabled so the robot stays running, but
    // retry periodically in case the port was just opened and the firmware
    // hasn't finished its startup sequence yet.
    QElapsedTimer timer;
    timer.start();
    int lastSendMs = 0;
    while (timer.elapsed() < kProbeTimeoutMs && !identified) {
        int elapsed = static_cast<int>(timer.elapsed());
        if (elapsed > 300 && elapsed - lastSendMs >= 500) {
            probe.send(zowi::makeCommand(zowi::Command::GetProgramId));
            lastSendMs = elapsed;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    probe.disconnect();
    // Drain any queued callbacks from the reader thread that captured
    // references to local variables before the probe goes out of scope.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    return identified ? port : QString();
#endif
}

// --- Connection actions -----------------------------------------------------

void RobotController::startScan()
{
    if (!m_backend) return;
    // Scanning is a Bluetooth-only concept.
    if (m_backendKind != Bluetooth) useBluetoothBackend();
    m_scanning = true;
    emit scanningChanged();
    m_backend->startDiscovery();
}

void RobotController::stopScan()
{
    if (!m_backend) return;
    m_backend->stopDiscovery();
    m_scanning = false;
    emit scanningChanged();
}

void RobotController::connectToDevice(const QString &address)
{
    if (address.isEmpty()) return;
    // A fresh attempt leaves demo-after-timeout and re-arms the watchdog.
    m_connectTimedOut = false;
    if (m_backendKind != Bluetooth) useBluetoothBackend();
    m_deviceAddress = address;
    setConnecting(true);
    m_backend->connect(address.toStdString());
    emit deviceChanged();
}

void RobotController::connectUsb(const QString &port)
{
    // Enter the "connecting" state *before* probing: when no port is known,
    // auto-detection runs probeZowiOnPort(), which can block for up to
    // kProbeTimeoutMs (6 s) while pumping events. `connecting` must already be
    // true so the UI shows the wait cursor during that window.
    setConnecting(true);
    QString target = port;
    if (target.isEmpty()) target = m_usbPort;
    if (target.isEmpty()) {
        // Snapshot to guard against reentrancy (same reason as refreshTransports).
        const auto ports = m_knownUsbPorts;
        for (const auto &p : ports) {
            // User-initiated action: allow re-probe even if already probed in
            // background polling (e.g. the previous probe may have failed due
            // to the robot still being in the bootloader after DTR reset).
            m_probedUsbPorts.removeAll(p);
            target = probeZowiOnPort(p);
            if (!target.isEmpty()) break;
        }
    }
    if (target.isEmpty()) {
        setConnecting(false);
        emit errorOccurred(tr("No USB robot detected"));
        return;
    }
    // A fresh attempt leaves demo-after-timeout and re-arms the watchdog.
    m_connectTimedOut = false;
    if (m_backendKind != Usb) useSerialBackend();
    m_usbPort = target;
    m_deviceAddress = target;
    // connecting was set at the top; the setter is guarded so this is a no-op.
    // The serial backend opens the TTY synchronously and reports failure by
    // return value only (no callback), so handle it here: otherwise
    // m_connecting would stay true until the watchdog fires.
    if (!m_backend->connect(target.toStdString())) {
        emit errorOccurred(tr("Could not open the USB connection"));
        setConnecting(false);
        return;
    }
    emit deviceChanged();
}

void RobotController::copyText(const QString &text)
{
    if (QGuiApplication::clipboard())
        QGuiApplication::clipboard()->setText(text);
}

void RobotController::disconnectFromDevice()
{
    if (!m_backend) return;
    m_backend->disconnect();
    setConnecting(false);
    m_deviceAddress.clear();
    m_deviceName.clear();
    if (!m_appId.isEmpty()) {
        m_appId.clear();
        emit appIdChanged();
    }
    emit deviceChanged();
}

void RobotController::unpairDevice(const QString &address)
{
    if (!m_backend || address.isEmpty()) return;
    m_backend->unpair(address.toStdString());
}

void RobotController::sendData(const QString &data)
{
    if (!m_backend) return;
    qDebug() << "robot tx:" << data.trimmed();
    m_backend->send(data.toStdString());
}

void RobotController::restoreFirmware(const QString &firmwarePath)
{
    // Early-failure helper: report through the dedicated restore signals so the
    // UI leaves the "restoring" state even when we never reach the upload.
    auto failEarly = [this](const QString &msg) {
        setRestoring(true);
        setRestoring(false);
        emit firmwareRestoreFinished(false, msg);
    };

    if (firmwarePath.isEmpty()) {
        failEarly(tr("Firmware path is empty"));
        return;
    }

    if (!m_backend || !m_backend->isConnected()) {
        failEarly(tr("Firmware restore failed"));
        return;
    }

    // Remember the target address before touching the connection: reconnecting
    // clears the backend's cached address. For USB the address is the TTY path,
    // which is cleared by onConnectionChanged(false) on every reconnect, so fall
    // back to the known USB port rather than trusting m_deviceAddress (which may
    // be empty after a reconnect even though the link is up).
    std::string targetAddress = m_deviceAddress.toStdString();
    if (targetAddress.empty() && m_backendKind == Usb) {
        targetAddress = m_usbPort.toStdString();
        if (targetAddress.empty() && !m_knownUsbPorts.isEmpty())
            targetAddress = m_knownUsbPorts.value(0).toStdString();
    }
    if (targetAddress.empty()) {
        failEarly(tr("Firmware restore failed"));
        return;
    }

    // Convert qrc:/ path to a temporary file for the firmware library
    QString localPath = firmwarePath;
    if (firmwarePath.startsWith("qrc:/") || firmwarePath.startsWith(":/")) {
        // QFile understands the ":/..." resource syntax but NOT the "qrc:/..."
        // URL syntax used by QML, so normalise it before opening.
        QString resourcePath = firmwarePath;
        if (resourcePath.startsWith("qrc:/"))
            resourcePath.remove(0, 3); // "qrc:/foo" -> ":/foo"

        QTemporaryFile tmpFile(QDir::tempPath() + "/zowi_firmware_XXXXXX.hex");
        // QTemporaryFile deletes the file when it goes out of scope; disable
        // that so the extracted HEX survives until stk500UploadFirmware() reads
        // it. It is removed manually at the end of this function.
        tmpFile.setAutoRemove(false);
        if (!tmpFile.open()) {
            failEarly(tr("Failed to create temporary firmware file"));
            return;
        }
        QFile resFile(resourcePath);
        if (!resFile.open(QIODevice::ReadOnly)) {
            failEarly(tr("Failed to open firmware resource: %1").arg(resourcePath));
            return;
        }
        tmpFile.write(resFile.readAll());
        tmpFile.close();
        resFile.close();
        localPath = tmpFile.fileName();
    }

    // Enter the restoring state and notify listeners (Phase 2).
    setRestoring(true);
    emit firmwareRestoreStarted();

    // Carry the context to the post-upload continuation (runs on the GUI thread).
    m_restoreLocalPath = localPath;
    m_restoreOriginalPath = firmwarePath;
    m_restoreTarget = QString::fromStdString(targetAddress);
    m_restoreIsUsb = (m_backendKind == Usb);

    // Phase 3: battery check BEFORE the upload, mirroring the CLI flow. The
    // running firmware reports its level via &&B; wait briefly for it, then if
    // it is below the (configurable) threshold, ask the user to confirm before
    // touching the robot. The confirmation is asynchronous (we cannot block the
    // GUI thread): we defer the actual restore to proceedWithRestore(), which is
    // called either immediately (battery ok) or from confirmRestoreBattery()
    // once the user decides.
    float battery = -1.0f;
    {
        QElapsedTimer batteryTimer;
        batteryTimer.start();
        while (batteryTimer.elapsed() < 2000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            if (m_battery >= 0.0f) { battery = m_battery; break; }
        }
    }

    const bool batteryLow = m_simulateLowBattery ||
                            (battery >= 0.0f && battery < m_lowBatteryThreshold);
    if (batteryLow) {
        m_batteryPending = true;
        emit firmwareRestoreBatteryLow(m_simulateLowBattery ? 20.0f : battery);
        // Defer: confirmRestoreBattery() will call proceedWithRestore() or abort.
        return;
    }

    proceedWithRestore();
}

void RobotController::proceedWithRestore()
{
    const bool isUsb = m_restoreIsUsb;
    const QString target = m_restoreTarget;
    bool stable = false;

    // Arm firmware-upload routing BEFORE any reconnect/reset work so we do not
    // send control polling frames (&&N/&&A/&&B) into the bootloader window.
    // This also captures any early bootloader bytes emitted right after reset.
    m_firmwareInstaller.enable();

    if (isUsb) {
        m_backend->disconnect();
        m_backend->setAutoReconnect(false);
#ifdef ZOWI_HAVE_SERIAL
        // The Optiboot USB bootloader runs at a different baud than the running
        // firmware (usb_baud); switch to the bootloader baud before the reset so
        // the reopened link is already at the speed the bootloader expects.
        if (auto *serial = dynamic_cast<SerialBackend *>(m_backend.get())) {
            serial->setBaudRate(m_usbBootloaderBaud);
            // Flashing drives the bootloader explicitly via pulseReset(); do not
            // add the control-connection boot delay.
            serial->setBootDelayMs(0);
        }
        const bool connectOk = m_backend->connect(target.toStdString());
        // The serial backend opens synchronously. Pulse DTR AFTER the port is
        // open to force the MCU into the bootloader: the implicit auto-reset on
        // (re)open is not reliable (DTR is usually already high from the previous
        // session, so there is no edge through the coupling capacitor), and
        // calling pulseReset() before connect() is a no-op (the fd is closed).
        // A fresh reset right before the sync maximizes the bootloader window.
        if (connectOk && m_backend->isConnected()) {
            if (auto *serial = dynamic_cast<SerialBackend *>(m_backend.get()))
                serial->pulseReset();
        }
        // Match the CLI timing: give the MCU a brief moment to land in the
        // bootloader before sending the first STK_GET_SYNC. If we send too
        // early (while reset is still in progress), the first sync attempt can
        // burn most of the bootloader window waiting for a reply timeout.
        {
            QElapsedTimer settle;
            settle.start();
            while (settle.elapsed() < 300)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        }
        // The serial backend opens synchronously; upload immediately to catch
        // the short post-reset bootloader window.
        stable = connectOk && m_backend->isConnected();
#else
        const bool connectOk = m_backend->connect(target.toStdString());
        stable = connectOk && m_backend->isConnected();
#endif
    } else {
        // First tear down the existing SPP link cleanly. Reconnecting on top of
        // a socket that is still closing triggers BlueZ "Cannot connect to
        // profile/service". disconnect() blocks until the socket reaches the
        // Unconnected state (up to 2 s) and clears the cached address.
        m_backend->setAutoReconnect(false);
        m_backend->disconnect();

        // Give BlueZ a moment to fully release the RFCOMM channel and let the
        // robot notice the drop before we reconnect to trigger the reset.
        {
            QElapsedTimer settle;
            settle.start();
            while (settle.elapsed() < 800)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }

        m_backend->setAutoReconnect(true, 100);
        m_backend->connect(target.toStdString());

        // Wait for a stable connection after the reset cycle (up to ~10 s). The
        // link may briefly drop while the robot reboots; the backend reconnects.
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 10000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            if (m_backend->isConnected()) {
                // Give the link a moment to settle past the reset cycle.
                QElapsedTimer settle;
                settle.start();
                while (settle.elapsed() < 300)
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
                if (m_backend->isConnected()) { stable = true; break; }
            }
        }
    }

    if (!stable) {
        // Could not reach the bootloader; finish now.
        m_firmwareInstaller.clear();
        finishRestore(false);
        return;
    }

    // The STK500 upload runs HERE, on the GUI thread. The backend's socket is
    // not thread-safe (it owns a QSocketNotifier bound to the GUI thread), so we
    // must not call send/receive from a worker thread. The progress callback
    // emits between pages and the pump pumps the event loop, so the progress
    // bar keeps updating and the UI stays responsive for the duration of the
    // upload (this is the same approach that already worked in Phase 2).
    const bool ok = m_firmwareInstaller.upload(
        m_restoreLocalPath.toStdString(),
        "stk",
        [this](const std::vector<uint8_t> &data) -> bool {
            return m_backend && m_backend->send(std::string(data.begin(), data.end()));
        },
        []() { QCoreApplication::processEvents(QEventLoop::AllEvents, 5); },
        [this](int percent, std::size_t written, std::size_t total) {
            emit firmwareRestoreProgress(percent, static_cast<int>(written), static_cast<int>(total));
        }
    );

    continueAfterUpload(ok);
}

void RobotController::setRestoring(bool value)
{
    if (m_restoring == value) return;
    m_restoring = value;
    emit restoringChanged();
}

void RobotController::continueAfterUpload(bool ok)
{
    const bool isUsb = m_restoreIsUsb;
    const QString target = m_restoreTarget;

    m_backend->setAutoReconnect(false);

    // For USB, the link was reopened at the bootloader baud; restore the normal
    // operating baud and reconnect to the running firmware so the app regains a
    // usable session after the upload.
    if (isUsb) {
        m_backend->disconnect();
#ifdef ZOWI_HAVE_SERIAL
        if (auto *serial = dynamic_cast<SerialBackend *>(m_backend.get()))
            serial->setBaudRate(m_usbBaud);
#endif
        m_deviceAddress = target;
        connectUsb(target);
    }

    const bool uploadOk = ok;

    // (The low-battery confirmation now happens BEFORE the upload, in
    // restoreFirmware(), mirroring the CLI's pre-upload battery check.)
    finishRestore(uploadOk);
}

void RobotController::confirmRestoreBattery(bool proceed)
{
    if (!m_batteryPending) return;
    m_batteryPending = false;
    if (proceed)
        proceedWithRestore();
    else
        finishRestore(false);
}

void RobotController::finishRestore(bool success)
{
    const QString localPath = m_restoreLocalPath;
    const QString originalPath = m_restoreOriginalPath;

    const QString resultMsg = success ? tr("Firmware restored successfully")
                                       : tr("Firmware restore failed");

    // Clean up temporary file if we created one.
    if (localPath != originalPath)
        QFile::remove(localPath);
    m_restoreLocalPath.clear();
    m_restoreOriginalPath.clear();

    // Leave the restoring state and report the outcome through dedicated signals.
    setRestoring(false);
    emit firmwareRestoreFinished(success, resultMsg);
}
