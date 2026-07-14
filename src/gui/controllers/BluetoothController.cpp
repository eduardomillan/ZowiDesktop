#include "BluetoothController.h"

#include <algorithm>
#include <cmath>
#include <string>

#ifdef Q_OS_WIN
#include "qt_bluetooth_backend.h"
#else
#include "qt_bluetooth_backend.h"
#endif

BluetoothController::BluetoothController(QObject *parent)
    : QObject(parent)
{
    auto backend = std::make_unique<zowi::QtBluetoothBackend>();

    backend->onDeviceFound([this](const zowi::DeviceInfo &info) {
        emit deviceDiscovered(
            QString::fromStdString(info.name),
            QString::fromStdString(info.address));
    });

    backend->onConnectionChanged([this](bool connected) {
        m_connected = connected;
        emit connectionChanged();
        if (!connected) {
            m_deviceName.clear();
            m_deviceAddress.clear();
            m_battery = -1.0f;
            emit deviceChanged();
            emit batteryChanged();
        }
    });

    backend->onDataReceived([this](const std::string &data) {
        m_rxBuffer += data;
        parseIncoming();
        emit dataReceived(QString::fromStdString(data));
    });

    backend->onError([this](const std::string &msg) {
        emit errorOccurred(QString::fromStdString(msg));
    });

    backend->onUnpairResult([this](bool ok, const std::string &msg) {
        emit unpairFinished(ok, QString::fromStdString(msg));
    });

    backend->BluetoothApi::onScanFinished([this]() {
        m_scanning = false;
        emit scanningChanged();
        emit scanFinished();
    });

    m_backend = std::move(backend);
}

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

bool BluetoothController::isConnected() const
{
    return m_backend && m_backend->isConnected();
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

void BluetoothController::startScan()
{
    if (!m_backend) return;
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
    if (!m_backend) return;
    m_deviceAddress = address;
    m_backend->connect(address.toStdString());
    emit deviceChanged();
}

void BluetoothController::disconnectFromDevice()
{
    if (!m_backend) return;
    m_backend->disconnect();
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
