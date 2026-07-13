#include "serial_bluetooth_backend.h"

#include <cerrno>

namespace zowi {

namespace {
constexpr speed_t kBootBaud = B9600;
}

SerialBluetoothBackend::SerialBluetoothBackend(QObject *parent)
    : QObject(parent)
{
}

SerialBluetoothBackend::~SerialBluetoothBackend()
{
    disconnect();
}

bool SerialBluetoothBackend::connect(const std::string &ttyPath)
{
    disconnect();

    m_fd = ::open(ttyPath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) {
        m_lastError = std::string("Failed to open ") + ttyPath + ": " + strerror(errno);
        return false;
    }

    struct termios tty {};
    if (tcgetattr(m_fd, &tty) != 0) {
        m_lastError = std::string("tcgetattr failed: ") + strerror(errno);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    cfsetospeed(&tty, kBootBaud);
    cfsetispeed(&tty, kBootBaud);

    tty.c_cflag &= ~PARENB;        // 8N1
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;       // no hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        m_lastError = std::string("tcsetattr failed: ") + strerror(errno);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    ::tcflush(m_fd, TCIOFLUSH);

    // Auto-reset the MCU into the bootloader. On the ZUM BT-328 the reset is
    // triggered by the HC-05 STATE pin when the SPP connection comes up (i.e. when
    // this TTY is opened / rfcomm-bound), so the caller must send STK_GET_SYNC
    // immediately afterwards, racing the bootloader's short post-reset window.
    // A brief DTR pulse is also sent for boards that wire DTR to RESET.
    pulseReset();

    // Minimal settle so the bootloader UART is ready; we must not dawdle here or
    // the bootloader's wait window expires before STK_GET_SYNC is sent.
    ::usleep(50 * 1000);

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    QObject::connect(m_notifier, &QSocketNotifier::activated, this, &SerialBluetoothBackend::onReadyRead);

    if (m_onConnect) m_onConnect(true);
    return true;
}

void SerialBluetoothBackend::disconnect()
{
    if (m_notifier) {
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    if (m_onConnect) m_onConnect(false);
}

bool SerialBluetoothBackend::send(const std::string &data)
{
    if (m_fd < 0) return false;
    const char *p = data.data();
    std::size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t n = ::write(m_fd, p, remaining);
        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            m_lastError = std::string("write failed: ") + strerror(errno);
            return false;
        }
        p += n;
        remaining -= static_cast<std::size_t>(n);
    }
    return true;
}

void SerialBluetoothBackend::onReadyRead()
{
    if (m_fd < 0) return;
    char buf[512];
    ssize_t n;
    while ((n = ::read(m_fd, buf, sizeof(buf))) > 0) {
        if (m_onData) m_onData(std::string(buf, static_cast<std::size_t>(n)));
    }
}

void SerialBluetoothBackend::pulseReset()
{
    if (m_fd < 0) return;
    int dtr = TIOCM_DTR;
    // Ensure DTR starts high, then drive it low (the falling edge resets the
    // MCU through the coupling capacitor), then back high.
    ::ioctl(m_fd, TIOCMBIS, &dtr);
    ::usleep(10 * 1000);
    ::ioctl(m_fd, TIOCMBIC, &dtr);
    ::usleep(50 * 1000);
    ::ioctl(m_fd, TIOCMBIS, &dtr);
    ::usleep(10 * 1000);
}

} // namespace zowi
