#pragma once

#include <zowi/bluetooth_api.h>
#include <QObject>
#include <QSocketNotifier>

#include <string>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

namespace zowi {

// Bluetooth backend that talks to the robot over an RFCOMM TTY
// (e.g. /dev/rfcomm0, created with `rfcomm bind`). Opening the serial port
// pulses the DTR line, which on the ZUM BT-328 board auto-resets the ATmega
// into its bootloader — exactly like programming an Arduino over its USB/UART.
class SerialBluetoothBackend : public QObject, public BluetoothApi {
    Q_OBJECT
public:
    explicit SerialBluetoothBackend(QObject *parent = nullptr);
    ~SerialBluetoothBackend() override;

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

private slots:
    void onReadyRead();

private:
    // Pulse DTR low then high to trigger the Arduino auto-reset into the
    // bootloader. The ZUM board couples DTR to RESET through a capacitor, so a
    // falling edge on DTR produces a reset pulse.
    void pulseReset();

    int m_fd = -1;
    QSocketNotifier *m_notifier = nullptr;
    std::string m_lastError;
};

} // namespace zowi
