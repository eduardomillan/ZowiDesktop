#pragma once

#include <zowi/bluetooth_api.h>
#include <QObject>
#include <QSocketNotifier>

#include <string>
#include <vector>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

namespace zowi {

// Bluetooth backend that talks to the robot over a USB serial TTY
// (e.g. /dev/ttyUSB0). Opening the port alone does NOT reset the robot; callers
// that want to enter the bootloader must call pulseReset() explicitly before
// connect() (used by firmware flashing). Control connections connect() without
// resetting, so the running firmware keeps handling commands.
class SerialBluetoothBackend : public QObject, public BluetoothApi {
    Q_OBJECT
public:
    explicit SerialBluetoothBackend(QObject *parent = nullptr);
    ~SerialBluetoothBackend() override;

    // Set the baud rate used when opening the TTY. Must be called before
    // connect(). Defaults to 9600 (the RFCOMM/ZUM BT-328 bootloader rate).
    // Optiboot over a USB serial link typically uses 57600 or 115200.
    void setBaudRate(int baud) { m_baud = baud; }
    int baudRate() const { return m_baud; }

    // Time to wait after opening the TTY before the firmware is ready to handle
    // commands. On a USB-serial link the MCU resets when the port is opened and
    // the bootloader runs for a moment before handing control to the firmware,
    // so callers must let that settle. Defaults to 5000ms. Firmware flashing
    // sets this to 0 (it drives the bootloader explicitly via pulseReset()).
    void setBootDelayMs(int ms) { m_bootDelayMs = ms; }
    int bootDelayMs() const { return m_bootDelayMs; }

    // Enumerate serial ports likely to be a robot's USB/UART link
    // (/dev/ttyUSB* and /dev/ttyACM*). Static so callers can list ports
    // without owning a backend instance.
    static std::vector<std::string> listSerialPorts();

    bool init() override { return true; }
    void startDiscovery() override {}
    void stopDiscovery() override {}
    bool connect(const std::string &address) override;
    void disconnect() override;
    bool send(const std::string &data) override;
    bool isConnected() const override { return m_fd >= 0; }
    std::string lastError() const override { return m_lastError; }
    void setAutoReconnect(bool, int = 3000) override {}
    void unpair(const std::string &) override {
        if (m_onUnpairResult) {
            auto cb = std::move(m_onUnpairResult);
            cb(false, "Unpair not supported on serial backend");
        }
    }

    // Pulse DTR low then high to trigger the Arduino auto-reset into the
    // bootloader. The ZUM board couples DTR to RESET through a capacitor, so a
    // falling edge on DTR produces a reset pulse. Callers must invoke this
    // explicitly *before* connect() only when flashing firmware; a normal
    // control connection must NOT reset the robot, or it would drop into the
    // bootloader and ignore control commands (e.g. rename).
    void pulseReset();

private slots:
    void onReadyRead();

private:

    int m_fd = -1;
    int m_baud = 9600;
    int m_bootDelayMs = 5000;
    QSocketNotifier *m_notifier = nullptr;
    std::string m_lastError;
};

} // namespace zowi
