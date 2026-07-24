#pragma once

#include <zowi/bluetooth_api.h>
#include <QObject>

#include <windows.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace zowi {

// Windows serial backend that talks to the robot over a COM port
// (e.g. COM3 for a USB-serial adapter). Mirrors the POSIX SerialBluetoothBackend
// but uses the Win32 API (CreateFile / ReadFile / WriteFile) instead of termios.
class WinSerialBackend : public QObject, public BluetoothApi {
    Q_OBJECT
public:
    explicit WinSerialBackend(QObject *parent = nullptr);
    ~WinSerialBackend() override;

    // Set the baud rate used when opening the COM port. Must be called before
    // connect(). Defaults to 9600 (the RFCOMM/ZUM BT-328 bootloader rate).
    // Optiboot over a USB serial link typically uses 57600 or 115200.
    void setBaudRate(int baud) { m_baud = baud; }
    int baudRate() const { return m_baud; }

    // Time to wait after opening the COM port before the firmware is ready to
    // handle commands. On a USB-serial link the MCU resets when the port is
    // opened and the bootloader runs for a moment before handing control to
    // the firmware, so callers must let that settle. Defaults to 5000ms.
    void setBootDelayMs(int ms) { m_bootDelayMs = ms; }
    int bootDelayMs() const { return m_bootDelayMs; }

    // Enumerate COM ports likely to be a robot's USB/serial link.
    // Static so callers can list ports without owning a backend instance.
    static std::vector<std::string> listSerialPorts();

    // Whether to keep DTR asserted while connected. Disable for probing so
    // the robot does not reset when the port is opened (useful on Windows
    // where opening a COM port automatically asserts DTR). Defaults to true.
    void setDtrEnabled(bool on) { m_dtrEnabled = on; }
    bool dtrEnabled() const { return m_dtrEnabled; }

    // Pulse DTR low then high to trigger the Arduino auto-reset into the
    // bootloader. Same as the POSIX backend: DTR falling edge resets via
    // the coupling capacitor on the ZUM board.
    void pulseReset();

    // BluetoothApi
    bool init() override { return true; }
    void startDiscovery() override {}
    void stopDiscovery() override {}
    bool connect(const std::string &address) override;
    void disconnect() override;
    bool send(const std::string &data) override;
    bool isConnected() const override { return m_handle != INVALID_HANDLE_VALUE; }
    std::string lastError() const override { return m_lastError; }
    void setAutoReconnect(bool, int = 3000) override {}
    void unpair(const std::string &) override {
        if (m_onUnpairResult) {
            auto cb = std::move(m_onUnpairResult);
            cb(false, "Unpair not supported on serial backend");
        }
    }

private:
    void readLoop();

    HANDLE m_handle = INVALID_HANDLE_VALUE;
    int m_baud = 9600;
    int m_bootDelayMs = 5000;
    bool m_dtrEnabled = true;
    std::string m_lastError;
    std::atomic<bool> m_reading{false};
    std::thread m_readThread;
};

} // namespace zowi
