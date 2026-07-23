#include "cli_util.h"
#include "cli_state.h"

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#ifndef _WIN32
#include <unistd.h>
#include <termios.h>
#include <csignal>
#include <sys/select.h>
#endif

#include <zowi/stk500v1.h>
#include <zowi/robot_commands.h>
#include <zowi/protocol.h>
#ifdef ZOWI_HAVE_SERIAL
#include <serial_bluetooth_backend.h>
#endif

namespace zowi_cli {

bool waitUntil(QCoreApplication &qtApp, int timeoutMs,
               const std::function<bool()> &predicate)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        qtApp.processEvents();
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

#ifndef _WIN32
bool discoverDevice(QCoreApplication &qtApp, zowi::QtBluetoothBackend &bt,
                    const std::string &address, int timeoutMs)
{
    bool found = false;
    bt.onDeviceFound([&](const zowi::DeviceInfo &dev) {
        if (dev.address == address) found = true;
    });
    bt.startDiscovery();
    bool ok = waitUntil(qtApp, timeoutMs, [&]() { return found; });
    bt.stopDiscovery();
    bt.onDeviceFound(nullptr);
    return ok;
}
#endif

// ── Interactive keyboard input (raw terminal mode) ───────────
#ifndef _WIN32
static struct termios g_origTermios;
static bool g_rawMode = false;

bool enableRawMode()
{
    if (tcgetattr(g_stdinFd, &g_origTermios) == -1) return false;
    struct termios raw = g_origTermios;
    raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(g_stdinFd, TCSAFLUSH, &raw) == -1) return false;
    g_rawMode = true;
    return true;
}

void disableRawMode()
{
    if (g_rawMode) {
        tcsetattr(g_stdinFd, TCSAFLUSH, &g_origTermios);
        g_rawMode = false;
    }
}
#else
bool enableRawMode() { return false; }
void disableRawMode() {}
#endif

#ifndef _WIN32
std::string readKey()
{
    unsigned char c;
    ssize_t n = read(g_stdinFd, &c, 1);
    if (n <= 0) return "";

    if (c == 3) return "quit";  // Ctrl-C
    if (c != 0x1b) {
        if (c == 'w' || c == 'W') return "up";
        if (c == 's' || c == 'S') return "down";
        if (c == 'a' || c == 'A') return "left";
        if (c == 'd' || c == 'D') return "right";
        if (c == 'q' || c == 'Q') return "turn_left";
        if (c == 'e' || c == 'E') return "turn_right";
        if (c == '+') return "speed_up";
        if (c == '-') return "speed_down";
        return "";
    }

    // ESC (0x1b): could be a standalone ESC (quit) or the start of an escape
    // sequence (arrow keys send ESC [ A/B/C/D or ESC O A/B/C/D).
    auto readByte = [](int ms) -> int {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(g_stdinFd, &fds);
        struct timeval tv{0, ms * 1000};
        if (select(g_stdinFd + 1, &fds, nullptr, nullptr, &tv) <= 0) return -1;
        unsigned char b;
        if (read(g_stdinFd, &b, 1) != 1) return -1;
        return b;
    };

    int b1 = readByte(50);
    if (b1 == '[' || b1 == 'O') {
        int b2 = readByte(50);
        switch (b2) {
            case 'A': return "up";
            case 'B': return "down";
            case 'C': return "right";
            case 'D': return "left";
            default: break;
        }
    }
    // Not a recognized escape sequence — treat standalone ESC as quit.
    return "quit";
}
#else
std::string readKey() { return ""; }
#endif

bool waitForRobotData(QCoreApplication &qtApp, int timeoutMs)
{
    return waitUntil(qtApp, timeoutMs, []() {
        std::lock_guard<std::mutex> lock(g_mtx);
        return !g_robotName.empty() && !g_appId.empty() && g_battery >= 0;
    });
}

bool waitForBatteryLevel(QCoreApplication &qtApp, int timeoutMs)
{
    return waitUntil(qtApp, timeoutMs, []() {
        std::lock_guard<std::mutex> lock(g_mtx);
        return g_battery >= 0;
    });
}

void requestRobotData(zowi::BluetoothApi &bt)
{
    if (g_debugLog) {
        std::cout << "robot tx: E (GetName)" << std::endl;
    }
    bt.send(zowi::makeCommand(zowi::Command::GetName));
    if (g_debugLog) {
        std::cout << "robot tx: I (GetProgramId)" << std::endl;
    }
    bt.send(zowi::makeCommand(zowi::Command::GetProgramId));
    if (g_debugLog) {
        std::cout << "robot tx: B (GetBattery)" << std::endl;
    }
    bt.send(zowi::makeCommand(zowi::Command::GetBattery));
}

bool waitForAppId(QCoreApplication &qtApp, zowi::BluetoothApi &bt, int timeoutMs,
                   const std::string &previousAppId)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    auto nextPoll = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() < deadline) {
        qtApp.processEvents();
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            // The robot reboots into the new firmware after flashing. Wait until it
            // reports an app id that differs from the one it had before the upload.
            if (!g_appId.empty() && (previousAppId.empty() || g_appId != previousAppId)) {
                return true;
            }
        }

        if (g_connected && std::chrono::steady_clock::now() >= nextPoll) {
            bt.send(zowi::makeCommand(zowi::Command::GetProgramId));
            nextPoll = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Timed out. Accept if we at least received some app id (it may not have changed).
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        return !g_appId.empty();
    }
}

bool uploadFirmware(zowi::BluetoothApi &bt, QCoreApplication &qtApp,
                    const std::string &firmwarePath, const std::string &protocol)
{
    // Route incoming bytes to the raw bootloader buffer for the upload duration.
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        g_uploadMode = true;
        g_stkBuffer.clear();
    }

    zowi::BootloaderTransport transport;
    transport.send = [&](const std::vector<uint8_t> &data) {
        return bt.send(std::string(data.begin(), data.end()));
    };
    transport.receive = [&](std::vector<uint8_t> &out, std::size_t maxBytes) -> int {
        std::lock_guard<std::mutex> lock(g_mtx);
        if (g_stkBuffer.empty()) return 0;
        const std::size_t n = std::min(g_stkBuffer.size(), maxBytes);
        out.assign(g_stkBuffer.data(), g_stkBuffer.data() + n);
        g_stkBuffer.erase(0, static_cast<std::size_t>(n));
        return static_cast<int>(n);
    };
    transport.pump = [&]() { qtApp.processEvents(); };

    const bool ok = (protocol == "stk")
                        ? zowi::stk500UploadFirmware(transport, firmwarePath)
                        : zowi::zowiRawHexUploadFirmware(transport, firmwarePath);

    {
        std::lock_guard<std::mutex> lock(g_mtx);
        g_uploadMode = false;
    }

    return ok;
}

std::unique_ptr<zowi::BluetoothApi> prepareFlashBackend(
    const std::string &backendName, const std::string &address,
    const std::string &ttyOpt, int baud, std::string &connectTarget,
    std::string &boundTty)
{
    const bool useUsb    = (backendName == "usb");
    const bool useSerial = (useUsb || backendName == "serial" || !ttyOpt.empty());

#ifdef ZOWI_HAVE_SERIAL
    if (useSerial) {
        std::string tty = ttyOpt;
        if (tty.empty()) {
            if (useUsb) {
                auto ports = zowi::SerialBluetoothBackend::listSerialPorts();
                if (ports.empty()) {
                    std::cerr << "No USB serial ports found (/dev/ttyUSB*, /dev/ttyACM*).\n"
                              << "Plug in the robot and pass --tty /dev/ttyUSB0." << std::endl;
                    return nullptr;
                }
                tty = ports.front();
                std::cerr << "Using USB serial port " << tty << std::endl;
            } else {
                if (address.empty()) {
                    std::cerr << "No paired device found. Run 'connect' first." << std::endl;
                    return nullptr;
                }
                tty = "/dev/rfcomm0";
                std::system(("rfcomm bind 0 " + address + " 1").c_str());
                if (access(tty.c_str(), F_OK) != 0) {
                    std::cerr << "Failed to bind RFCOMM TTY " << tty
                              << " (need CAP_NET_ADMIN / root).\n"
                              << "Either run as root, or bind manually:\n"
                              << "  rfcomm bind 0 " << address << " 1\n"
                              << "and pass --tty /dev/rfcomm0" << std::endl;
                    return nullptr;
                }
                boundTty = tty;
            }
        }
        connectTarget = tty;
        auto backend = std::make_unique<zowi::SerialBluetoothBackend>();
        backend->setBaudRate(baud);
        return backend;
    }
#else
    if (useSerial) {
        std::cerr << "Serial/USB backend not available on this platform." << std::endl;
        return nullptr;
    }
#endif

    if (address.empty()) {
        std::cerr << "No paired device found. Run 'connect' first." << std::endl;
        return nullptr;
    }
    connectTarget = address;
#ifdef ZOWI_HAVE_NATIVE_BT
    return std::make_unique<zowi::NativeBluetoothBackend>();
#else
    return std::make_unique<zowi::QtBluetoothBackend>();
#endif
}

bool installFirmwareToPairedZowi(QCoreApplication &qtApp,
                                 zowi::BluetoothApi &bt,
                                 zowi::SessionStore &session,
                                 const std::string &actionLabel,
                                 const std::string &firmwarePath,
                                 int batteryTimeoutSeconds,
                                 int confirmationTimeoutSeconds,
                                 bool forceLowBattery,
                                 const std::string &protocol)
{
    resetRobotState();

    // The connection callback may have already fired inside bt.connect() (this
    // is the case for the serial/USB backend, which opens the port synchronously
    // and never reconnects). resetRobotState() cleared that flag, so seed it
    // from the backend's actual connection state before waiting.
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        if (bt.isConnected()) {
            g_connected = true;
            g_connectedOnce = true;
        }
    }

    // Enable upload mode early so any bootloader response arriving during the
    // reconnection phase is routed to g_stkBuffer instead of being parsed as a
    // protocol message.
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        g_uploadMode = true;
        g_stkBuffer.clear();
    }

    // Wait for a stable connection. On Bluetooth the first SPP connect triggers
    // the STATE-pin reset, which may briefly drop the link while the robot
    // reboots; the backend reconnects (100 ms interval) and onConnect fires
    // again. On serial/USB the connection is already open and stable.
    // Allow up to 10 s total.
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (!waitUntil(qtApp, 5000, []() {
                std::lock_guard<std::mutex> lock(g_mtx);
                return g_connectedOnce;
            })) {
            std::cerr << "Could not connect to the robot within the timeout." << std::endl;
            bt.disconnect();
            return false;
        }
        // Give the link a moment to settle after the reset cycle.
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (g_connected) break; // still connected → stable
        // Dropped again (reset cycle), loop to wait for reconnect.
    }
    if (!g_connected) {
        std::cerr << "Could not establish a stable connection to the robot." << std::endl;
        bt.disconnect();
        return false;
    }

    const bool bootloaderMode = (protocol == "raw" || protocol == "stk");
    if (!bootloaderMode) {
        const bool batteryAvailable = waitForBatteryLevel(qtApp, batteryTimeoutSeconds * 1000);
        if (batteryAvailable) {
            std::cout << "Battery reported: " << g_battery << "%" << std::endl;
            if (g_battery < kLowBatteryThreshold && !forceLowBattery) {
                std::cerr << "Battery is below the recommended " << kLowBatteryThreshold
                          << "% threshold. Re-run with --force-low-battery to continue." << std::endl;
                bt.disconnect();
                return false;
            }
            if (g_battery < kLowBatteryThreshold) {
                std::cout << "Continuing despite low battery because --force-low-battery was provided." << std::endl;
            }
        } else {
            std::cout << "Battery level was not received before the timeout. Continuing with " << actionLabel << "." << std::endl;
        }
    } else {
        std::cout << "Bootloader mode: skipping battery check and uploading immediately." << std::endl;
    }

    std::string previousAppId;
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        previousAppId = g_appId;
        g_appId.clear();
        g_dataReceived = false;
    }

    if (!uploadFirmware(bt, qtApp, firmwarePath, protocol)) {
        bt.disconnect();
        return false;
    }

    std::cout << "Waiting for the updated firmware to report its app ID..." << std::endl;
    if (!waitForAppId(qtApp, bt, confirmationTimeoutSeconds * 1000, previousAppId)) {
        std::cerr << "Firmware upload finished, but no app ID was received within "
                  << confirmationTimeoutSeconds << "s." << std::endl;
        bt.disconnect();
        return false;
    }

    if (!g_appId.empty()) {
        session.setString("activeZowiAppId", g_appId);
    }
    if (!g_robotName.empty()) {
        session.setString("activeZowiName", g_robotName);
    }
    if (g_battery >= 0) {
        session.setInt("activeZowiBattery", static_cast<int>(g_battery));
    }

    std::cout << actionLabel << " completed." << std::endl;
    std::cout << "  App ID:  " << (g_appId.empty() ? "(not received)" : g_appId) << std::endl;
    std::cout << "Session updated." << std::endl;

    bt.disconnect();
    return true;
}

} // namespace zowi_cli
