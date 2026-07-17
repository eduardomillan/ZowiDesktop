#include "BluetoothController.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include <QCoreApplication>
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
constexpr int kProbeTimeoutMs = 1800;

BluetoothController::Transport transportFromString(const QString &s)
{
    const QString v = s.trimmed().toLower();
    if (v == "usb") return BluetoothController::Usb;
    if (v == "bluetooth" || v == "bt") return BluetoothController::Bluetooth;
    return BluetoothController::Auto;
}
} // namespace

BluetoothController::BluetoothController(QObject *parent)
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

    // Start on the Bluetooth backend by default; detection may switch it.
    useBluetoothBackend();

    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &BluetoothController::pollTransports);
    m_pollTimer.start();

    // Initial availability snapshot + auto-detection.
    refreshTransports();
}

// --- Backend construction ---------------------------------------------------

void BluetoothController::useBluetoothBackend()
{
    if (m_backend && m_backendKind == Bluetooth) return;
    m_backend = std::make_unique<zowi::QtBluetoothBackend>();
    m_backendKind = Bluetooth;
    wireBackend();
    setActiveTransport(Bluetooth);
}

void BluetoothController::useSerialBackend()
{
#ifdef ZOWI_HAVE_SERIAL
    if (m_backend && m_backendKind == Usb) return;
    auto serial = std::make_unique<zowi::SerialBluetoothBackend>();
    serial->setBaudRate(m_usbBaud);
    m_backend = std::move(serial);
    m_backendKind = Usb;
    wireBackend();
    setActiveTransport(Usb);
#else
    useBluetoothBackend();
#endif
}

void BluetoothController::wireBackend()
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
        if (connected) {
            setConnecting(false);
        } else {
            m_deviceName.clear();
            m_deviceAddress.clear();
            m_battery = -1.0f;
            emit deviceChanged();
            emit batteryChanged();
        }
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

void BluetoothController::setActiveTransport(Transport t)
{
    if (m_activeTransport == t) return;
    m_activeTransport = t;
    emit activeTransportChanged();
}

// --- Incoming parsing -------------------------------------------------------

void BluetoothController::parseIncoming()
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

// --- Property getters -------------------------------------------------------

bool BluetoothController::isBluetoothAvailable() const
{
    return m_bluetoothAvailable;
}

bool BluetoothController::isUsbAvailable() const
{
    return m_usbAvailable;
}

int BluetoothController::transport() const
{
    return static_cast<int>(m_transport);
}

int BluetoothController::activeTransport() const
{
    return static_cast<int>(m_activeTransport);
}

bool BluetoothController::isConnected() const
{
    return m_backend && m_backend->isConnected();
}

bool BluetoothController::isConnecting() const
{
    return m_connecting;
}

void BluetoothController::setConnecting(bool value)
{
    if (m_connecting == value) return;
    m_connecting = value;
    emit connectingChanged();
}

bool BluetoothController::isScanning() const
{
    return m_scanning;
}

QString BluetoothController::deviceName() const
{
    return m_deviceName;
}

QString BluetoothController::deviceAddress() const
{
    return m_deviceAddress;
}

int BluetoothController::battery() const
{
    return m_battery >= 0.0f ? static_cast<int>(std::round(m_battery)) : -1;
}

void BluetoothController::setDeviceName(const QString &name)
{
    if (m_deviceName != name) {
        m_deviceName = name;
        emit deviceChanged();
    }
}

// --- Transport selection ----------------------------------------------------

void BluetoothController::setTransport(int transport)
{
    Transport t = static_cast<Transport>(transport);
    if (t != Auto && t != Bluetooth && t != Usb) return;
    if (m_transport == t) return;
    m_transport = t;
    emit transportChanged();

    // Persist the choice so it is honoured next launch.
    zowi::SessionStore session;
    const char *s = (t == Usb) ? "usb" : (t == Bluetooth ? "bluetooth" : "auto");
    session.setString("transport", s);

    // Switch the live backend immediately when the user is explicit and the
    // target transport is available (avoid tearing down an active connection).
    if (isConnected()) return;
    if (t == Usb && m_usbAvailable) useSerialBackend();
    else if (t == Bluetooth) useBluetoothBackend();
    else if (t == Auto) refreshTransports();
}

QStringList BluetoothController::listUsbPorts() const
{
    QStringList out;
#ifdef ZOWI_HAVE_SERIAL
    for (const auto &p : zowi::SerialBluetoothBackend::listSerialPorts())
        out << QString::fromStdString(p);
#endif
    return out;
}

void BluetoothController::refreshTransports()
{
    pollTransports();

    // Auto mode: pick the best available transport while disconnected.
    if (m_transport == Auto && !isConnected() && !m_connecting) {
        if (m_usbAvailable) {
            QString port = probeZowiOnPort(m_knownUsbPorts.value(0));
            if (!port.isEmpty()) {
                m_usbPort = port;
                useSerialBackend();
                return;
            }
        }
        if (m_bluetoothAvailable) {
            useBluetoothBackend();
            return;
        }
    } else if (m_transport == Usb && m_usbAvailable && !isConnected()) {
        useSerialBackend();
    } else if (m_transport == Bluetooth && !isConnected()) {
        useBluetoothBackend();
    }
}

void BluetoothController::pollTransports()
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
    if (changed) emit transportsChanged();
}

QString BluetoothController::probeZowiOnPort(const QString &port)
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

void BluetoothController::startScan()
{
    if (!m_backend) return;
    // Scanning is a Bluetooth-only concept.
    if (m_backendKind != Bluetooth) useBluetoothBackend();
    m_scanning = true;
    emit scanningChanged();
    m_backend->startDiscovery();
}

void BluetoothController::stopScan()
{
    if (!m_backend) return;
    m_backend->stopDiscovery();
    m_scanning = false;
    emit scanningChanged();
}

void BluetoothController::connectToDevice(const QString &address)
{
    if (address.isEmpty()) return;
    if (m_backendKind != Bluetooth) useBluetoothBackend();
    m_deviceAddress = address;
    setConnecting(true);
    m_backend->connect(address.toStdString());
    emit deviceChanged();
}

void BluetoothController::connectUsb(const QString &port)
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

void BluetoothController::disconnectFromDevice()
{
    if (!m_backend) return;
    m_backend->disconnect();
    setConnecting(false);
    m_deviceAddress.clear();
    m_deviceName.clear();
    emit deviceChanged();
}

void BluetoothController::unpairDevice(const QString &address)
{
    if (!m_backend || address.isEmpty()) return;
    m_backend->unpair(address.toStdString());
}

void BluetoothController::sendData(const QString &data)
{
    if (!m_backend) return;
    m_backend->send(data.toStdString());
}

void BluetoothController::restoreFirmware(const QString &firmwarePath)
{
    if (firmwarePath.isEmpty()) {
        emit errorOccurred(tr("Firmware path is empty"));
        return;
    }

    if (!m_backend || !m_backend->isConnected()) {
        emit errorOccurred(tr("Not connected to a Zowi robot"));
        return;
    }

    // Convert qrc:/ path to a temporary file for the firmware library
    QString localPath = firmwarePath;
    if (firmwarePath.startsWith("qrc:/") || firmwarePath.startsWith(":/")) {
        QTemporaryFile tmpFile(QDir::tempPath() + "/zowi_firmware_XXXXXX.hex");
        if (!tmpFile.open()) {
            emit errorOccurred(tr("Failed to create temporary firmware file"));
            return;
        }
        QFile resFile(firmwarePath);
        if (!resFile.open(QIODevice::ReadOnly)) {
            emit errorOccurred(tr("Failed to open firmware resource: %1").arg(firmwarePath));
            return;
        }
        tmpFile.write(resFile.readAll());
        tmpFile.close();
        resFile.close();
        localPath = tmpFile.fileName();
    }

    // Prepare transport for firmware upload
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
    transport.pump = [this]() {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    };

    // Enable upload mode so dataReceived routes to m_stkBuffer
    {
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        m_uploadMode = true;
        m_stkBuffer.clear();
    }

    // Use STK500v1 protocol (Optiboot) for firmware upload
    bool ok = zowi::stk500UploadFirmware(transport, localPath.toStdString());

    // Disable upload mode
    {
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        m_uploadMode = false;
    }

    if (ok) {
        emit errorOccurred(tr("Firmware restored successfully"));
    } else {
        emit errorOccurred(tr("Firmware restore failed"));
    }

    // Clean up temporary file if we created one
    if (localPath != firmwarePath) {
        QFile::remove(localPath);
    }
}

