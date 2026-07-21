#include "RobotController.h"
#include "SessionController.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>

#include "qt_bluetooth_backend.h"
#ifdef ZOWI_HAVE_SERIAL
#include "serial_bluetooth_backend.h"
#endif
#include <zowi/config_store.h>
#include <zowi/session_store.h>
#include <zowi/protocol.h>
#include <zowi/stk500v1.h>

namespace {
// How often (ms) to poll for USB port / Bluetooth adapter appearance.
constexpr int kPollIntervalMs = 2500;
// How long (ms) to wait for a Zowi to answer the identification handshake.
constexpr int kProbeTimeoutMs = 6000;

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
    m_dataPollTimer.setInterval(1000);
    connect(&m_dataPollTimer, &QTimer::timeout, this, &RobotController::requestRobotData);

    // Initial availability snapshot + auto-detection.
    refreshTransports();
    m_situation = computeSituation();
}

// --- Backend construction ---------------------------------------------------

void RobotController::useBluetoothBackend()
{
    if (m_backend && m_backendKind == Bluetooth) return;
    m_backend = std::make_unique<zowi::QtBluetoothBackend>();
    m_backendKind = Bluetooth;
    wireBackend();
    setActiveTransport(Bluetooth);
}

void RobotController::useSerialBackend()
{
#ifdef ZOWI_HAVE_SERIAL
    if (m_backend && m_backendKind == Usb) return;
    auto serial = std::make_unique<zowi::SerialBluetoothBackend>();
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

    m_backend->onDeviceFound([this](const zowi::DeviceInfo &info) {
        emit deviceDiscovered(
            QString::fromStdString(info.name),
            QString::fromStdString(info.address));
    });

    m_backend->onConnectionChanged([this](bool connected) {
        m_connected = connected;
        emit connectionChanged();
        qInfo() << "[conn] connected=" << connected << "kind=" << m_backendKind
                << "usbPort=" << m_usbPort << "deviceAddress=" << m_deviceAddress
                << "known=" << m_knownUsbPorts;
        if (connected) {
            setConnecting(false);
            // For USB the address is the TTY path; onConnectionChanged(false)
            // clears it, so restore it from the known USB port when the link
            // comes back up (m_deviceAddress may otherwise stay empty).
            if (m_backendKind == Usb && m_deviceAddress.isEmpty()) {
                if (!m_usbPort.isEmpty())
                    m_deviceAddress = m_usbPort;
                else if (!m_knownUsbPorts.isEmpty())
                    m_deviceAddress = m_knownUsbPorts.value(0);
                if (!m_deviceAddress.isEmpty())
                    emit deviceChanged();
            }
            // Ask the robot for its name, firmware id and battery. The firmware
            // only reports these on request, so start a periodic poll.
            requestRobotData();
            m_dataPollTimer.start();
            // If a Zowi is registered, tie its registration to the transport we
            // actually connected with so future launches honour it.
            {
                zowi::SessionStore session;
                const QString addr = QString::fromStdString(
                    session.getString("activeZowiDeviceAddress"));
                if (!addr.isEmpty())
                    persistRegistrationTransport(m_backendKind);
            }
        } else {
            m_dataPollTimer.stop();
            m_deviceName.clear();
            // For USB the "address" is the TTY path, which is stable across the
            // connect/disconnect cycle; keep it so registration and the UI can
            // read it (finishRegistration runs on the connection signal before
            // the C++-side restore below would repopulate it).
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
    });

    m_backend->onDataReceived([this](const std::string &data) {
        // Route data to STK500 buffer during firmware upload, otherwise parse normally
        {
            std::lock_guard<std::mutex> lock(m_uploadMutex);
            if (m_uploadMode) {
                m_stkBuffer += data;
            }
        }
        if (!m_uploadMode) {
            m_rxBuffer += data;
            parseIncoming();
        }
        qDebug() << "robot rx:" << QString::fromStdString(data).trimmed();
        emit dataReceived(QString::fromStdString(data));
    });

    m_backend->onError([this](const std::string &msg) {
        emit errorOccurred(QString::fromStdString(msg));
    });

    m_backend->onUnpairResult([this](bool ok, const std::string &msg) {
        emit unpairFinished(ok, QString::fromStdString(msg));
    });

    m_backend->onScanFinished([this]() {
        m_scanning = false;
        emit scanningChanged();
        emit scanFinished();
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
    // The robot frames responses as &&<token><payload>%% (e.g. &&B 85.0%%) and
    // can also send line-based messages (e.g. "B 85.0"). We only need the
    // battery level here; mirror the CLI parser in src/cli/main.cpp.
    bool updated = false;
    std::string &buf = m_rxBuffer;

    // &&B <value>%% form.
    auto amp = buf.find("&&B ");
    while (amp != std::string::npos) {
        auto end = buf.find("%%", amp);
        if (end == std::string::npos) break; // incomplete frame, wait for more
        std::string value = buf.substr(amp + 4, end - (amp + 4));
        try {
            float b = std::stof(value);
            if (b != m_battery) { m_battery = b; updated = true; }
        } catch (...) {}
        buf.erase(0, end + 2);
        amp = buf.find("&&B ");
    }

    // &&E <name>%% form.
    auto ampE = buf.find("&&E ");
    while (ampE != std::string::npos) {
        auto end = buf.find("%%", ampE);
        if (end == std::string::npos) break; // incomplete frame, wait for more
        std::string value = buf.substr(ampE + 4, end - (ampE + 4));
        if (value != m_deviceName.toStdString()) {
            m_deviceName = QString::fromStdString(value);
            emit deviceChanged();
        }
        buf.erase(0, end + 2);
        ampE = buf.find("&&E ");
    }

    // &&I <appId>%% form: firmware / program id reported by the robot.
    auto ampI = buf.find("&&I ");
    while (ampI != std::string::npos) {
        auto end = buf.find("%%", ampI);
        if (end == std::string::npos) break; // incomplete frame, wait for more
        std::string value = buf.substr(ampI + 4, end - (ampI + 4));
        if (QString::fromStdString(value) != m_appId) {
            m_appId = QString::fromStdString(value);
            emit appIdChanged();
            // Persist the firmware id so the app remembers which firmware this
            // Zowi is running (used later to surface it in the UI).
            if (m_session)
                m_session->saveActiveZowiAppId(m_appId);
        }
        buf.erase(0, end + 2);
        ampI = buf.find("&&I ");
    }

    // Line-based "B <value>" / "N <name>" forms (value until newline).
    auto nl = buf.find('\n');
    while (nl != std::string::npos) {
        std::string line = buf.substr(0, nl);
        buf.erase(0, nl + 1);
        if (line.size() > 2 && line[0] == 'B' && line[1] == ' ') {
            try {
                float b = std::stof(line.substr(2));
                if (b != m_battery) { m_battery = b; updated = true; }
            } catch (...) {}
        } else if (line.size() > 2 && line[0] == 'N' && line[1] == ' ') {
            std::string value = line.substr(2);
            if (value != m_deviceName.toStdString()) {
                m_deviceName = QString::fromStdString(value);
                emit deviceChanged();
            }
        }
        nl = buf.find('\n');
    }

    // Keep the buffer from growing unbounded.
    if (buf.size() > 512) buf.erase(0, buf.size() - 512);

    if (updated) emit batteryChanged();
}

void RobotController::requestRobotData()
{
    if (!m_connected || m_uploadMode) return;
    if (!m_backend) return;
    m_backend->send(zowi::makeCommand(zowi::Command::GetName));
    m_backend->send(zowi::makeCommand(zowi::Command::GetProgramId));
    m_backend->send(zowi::makeCommand(zowi::Command::GetBattery));
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
    emit connectingChanged();
    maybeEmitSituation();
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
        const char *s = (t == Usb) ? "usb" : (t == Bluetooth ? "bluetooth" : "auto");
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
        const char *s = (t == Usb) ? "usb" : (t == Bluetooth ? "bluetooth" : "auto");
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
        const char *s = (prev == Usb) ? "usb" : (prev == Bluetooth ? "bluetooth" : "auto");
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
    m_session->saveActiveZowiTransport(t == Usb ? "usb" : "bluetooth");
    maybeEmitSituation();
}

QStringList RobotController::listUsbPorts() const
{
    QStringList out;
#ifdef ZOWI_HAVE_SERIAL
    for (const auto &p : zowi::SerialBluetoothBackend::listSerialPorts())
        out << QString::fromStdString(p);
#endif
    return out;
}

void RobotController::refreshTransports()
{
    pollTransports();

    // Auto mode: Bluetooth is preferred. USB is only used as fallback when
    // no Bluetooth adapter is present.
    if (m_transport == Auto && !isConnected() && !m_connecting) {
        if (m_bluetoothAvailable) {
            useBluetoothBackend();
            return;
        }
        if (m_usbAvailable) {
            QString port = probeZowiOnPort(m_knownUsbPorts.value(0));
            if (!port.isEmpty()) {
                m_usbPort = port;
                useSerialBackend();
                return;
            }
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

    bool usbAvail = !ports.isEmpty();
    bool btAvail = zowi::QtBluetoothBackend::hasAdapter();

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
    maybeEmitSituation();
}

QString RobotController::probeZowiOnPort(const QString &port)
{
#ifndef ZOWI_HAVE_SERIAL
    Q_UNUSED(port)
    return QString();
#else
    if (port.isEmpty()) return QString();
    // Only handshake a given port once per session: opening it resets the
    // robot via DTR, so we must not do it repeatedly during polling.
    if (m_probedUsbPorts.contains(port)) return QString();
    m_probedUsbPorts << port;

    zowi::SerialBluetoothBackend probe;
    probe.setBaudRate(m_usbBaud);
    // The probe only opens the port to detect a robot; do not block 5s on the
    // boot delay here (polling would stall). The probe loop below already waits
    // for the firmware to come up after the DTR reset.
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

    // Give the freshly-reset bootloader a moment, then request the program id.
    QElapsedTimer timer;
    timer.start();
    bool sent = false;
    while (timer.elapsed() < kProbeTimeoutMs && !identified) {
        if (!sent && timer.elapsed() > 300) {
            probe.send(zowi::makeCommand(zowi::Command::GetProgramId));
            sent = true;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    probe.disconnect();
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
    if (m_backendKind != Bluetooth) useBluetoothBackend();
    m_deviceAddress = address;
    setConnecting(true);
    m_backend->connect(address.toStdString());
    emit deviceChanged();
}

void RobotController::connectUsb(const QString &port)
{
    QString target = port;
    if (target.isEmpty()) target = m_usbPort;
    if (target.isEmpty()) target = m_knownUsbPorts.value(0);
    if (target.isEmpty()) {
        emit errorOccurred(tr("No USB robot detected"));
        return;
    }
    if (m_backendKind != Usb) useSerialBackend();
    m_usbPort = target;
    m_deviceAddress = target;
    setConnecting(true);
    m_backend->connect(target.toStdString());
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

    if (isUsb) {
        m_backend->disconnect();
        m_backend->setAutoReconnect(false);
        // The Optiboot USB bootloader runs at a different baud than the running
        // firmware (usb_baud); switch to the bootloader baud before the reset so
        // the reopened link is already at the speed the bootloader expects.
        if (auto *serial = dynamic_cast<zowi::SerialBluetoothBackend *>(m_backend.get())) {
            serial->setBaudRate(m_usbBootloaderBaud);
            // Flashing drives the bootloader explicitly via pulseReset(); do not
            // add the control-connection boot delay.
            serial->setBootDelayMs(0);
        }
        // Reset into the bootloader before (re)opening the link so the upload
        // races the short post-reset window.
        if (auto *serial = dynamic_cast<zowi::SerialBluetoothBackend *>(m_backend.get()))
            serial->pulseReset();
        const bool connectOk = m_backend->connect(target.toStdString());
        // The serial backend opens synchronously; upload immediately to catch
        // the short post-reset bootloader window.
        stable = connectOk && m_backend->isConnected();
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
        finishRestore(false);
        return;
    }

    // The STK500 upload runs HERE, on the GUI thread. The backend's socket is
    // not thread-safe (it owns a QSocketNotifier bound to the GUI thread), so we
    // must not call send/receive from a worker thread. The progress callback
    // emits between pages and the pump pumps the event loop, so the progress
    // bar keeps updating and the UI stays responsive for the duration of the
    // upload (this is the same approach that already worked in Phase 2).
    zowi::BootloaderTransport transport;
    transport.send = [this](const std::vector<uint8_t> &data) -> bool {
        return m_backend && m_backend->send(std::string(data.begin(), data.end()));
    };
    transport.receive = [this](std::vector<uint8_t> &out, std::size_t maxBytes) -> int {
        if (!m_backend) return -1;
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        if (m_stkBuffer.empty()) return 0;
        const std::size_t n = std::min(m_stkBuffer.size(), maxBytes);
        out.assign(m_stkBuffer.data(), m_stkBuffer.data() + n);
        m_stkBuffer.erase(0, n);
        return static_cast<int>(n);
    };
    transport.pump = []() {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    };
    transport.progress = [this](int percent, std::size_t written, std::size_t total) {
        emit firmwareRestoreProgress(percent, static_cast<int>(written), static_cast<int>(total));
    };

    // Enable upload mode early so any bootloader traffic arriving during the
    // reset/reconnect cycle is routed to m_stkBuffer instead of the protocol
    // parser.
    {
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        m_uploadMode = true;
        m_stkBuffer.clear();
    }

    const bool ok = zowi::stk500UploadFirmware(transport, m_restoreLocalPath.toStdString());

    {
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        m_uploadMode = false;
    }

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
        if (auto *serial = dynamic_cast<zowi::SerialBluetoothBackend *>(m_backend.get()))
            serial->setBaudRate(m_usbBaud);
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

