#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <unistd.h>
#include <condition_variable>
#include <termios.h>
#include <csignal>
#include <sys/select.h>
#include <CLI/CLI.hpp>
#include <QCoreApplication>
#include <zowi/session_store.h>
#include <zowi/translation_engine.h>
#include <zowi/config_store.h>
#include <zowi/robot_commands.h>
#include <zowi/protocol.h>
#include <zowi/stk500v1.h>
#include <qt_bluetooth_backend.h>
#include <serial_bluetooth_backend.h>

static std::mutex g_mtx;
static std::condition_variable g_cv;
static std::string g_robotName;
static std::string g_appId;
static float g_battery = -1.0f;
static bool g_connected = false;
static bool g_dataReceived = false;
static bool g_finalAck = false;
static std::string g_dataBuffer;
// When true, incoming bytes are raw bootloader traffic, not the &&/N-U-B protocol.
static bool g_uploadMode = false;
static std::string g_stkBuffer;
static std::atomic<bool> g_quit{false};
static constexpr float kLowBatteryThreshold = 50.0f;
static constexpr const char *kFactoryFirmwarePath = "src/firmware/ZOWI_BASE_v2.hex";
static constexpr const char *kAlarmFirmwarePath = "src/firmware/ZOWI_Alarm_v2.hex";

static void resetRobotState()
{
    std::lock_guard<std::mutex> lock(g_mtx);
    g_robotName.clear();
    g_appId.clear();
    g_battery = -1.0f;
    g_connected = false;
    g_dataReceived = false;
    g_finalAck = false;
    g_dataBuffer.clear();
}

static std::string trimRobotMessage(const std::string &msg)
{
    std::string trimmed = msg;
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\n'), trimmed.end());
    return trimmed;
}

static void parseRobotMessageUnlocked(const std::string &msg)
{
    std::string trimmed = trimRobotMessage(msg);
    if (trimmed.empty()) return;

    // &&‑prefixed message: &&<cmd>[ <value>]
    if (trimmed.size() >= 4 && trimmed[0] == zowi::kMessagePrefix[0]
                             && trimmed[1] == zowi::kMessagePrefix[1]
                             && trimmed[3] == ' ') {
        char prefix = trimmed[2];
        std::string value = trimmed.substr(4);
        if (prefix == zowi::toChar(zowi::Command::GetName)) {
            g_robotName = value;
            g_dataReceived = true;
        } else if (prefix == zowi::toChar(zowi::Command::GetProgramId)) {
            g_appId = value;
            g_dataReceived = true;
        } else if (prefix == zowi::toChar(zowi::Command::GetBattery)) {
            try { g_battery = std::stof(value); } catch (...) {}
            g_dataReceived = true;
        } else if (prefix == zowi::toChar(zowi::Command::Ack) || prefix == zowi::toChar(zowi::Command::FinalAck)) {
            // Software ack (&&A) / final ack (&&F) from the robot. The firmware
            // sends &&F only after it has processed the command (e.g. after the
            // EEPROM write in 'R'), so it confirms the operation completed.
            g_finalAck = true;
            g_dataReceived = true;
        }
        return;
    }

    // Legacy line-based messages (old firmware, still parsed for compatibility).
    if (trimmed[0] == zowi::toChar(zowi::Command::LegacyName) && trimmed.size() > 2 && trimmed[1] == ' ') {
        g_robotName = trimmed.substr(2);
        g_dataReceived = true;
    } else if (trimmed[0] == zowi::toChar(zowi::Command::LegacyProgramId) && trimmed.size() > 2 && trimmed[1] == ' ') {
        g_appId = trimmed.substr(2);
        g_dataReceived = true;
    } else if (trimmed[0] == zowi::toChar(zowi::Command::LegacyBattery) && trimmed.size() > 2 && trimmed[1] == ' ') {
        try { g_battery = std::stof(trimmed.substr(2)); } catch (...) {}
        g_dataReceived = true;
    }
}

static void onDataReceived(const std::string &data)
{
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        if (g_uploadMode) {
            g_stkBuffer += data;
            return;
        }
    }

    std::lock_guard<std::mutex> lock(g_mtx);
    g_dataBuffer += data;

    // Robot protocol can arrive either as &&E <name>%% / &&I <appId>%% / &&B <battery>%%
    // or as line-based N / U / B messages.
    while (true) {
        auto start = g_dataBuffer.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            g_dataBuffer.clear();
            break;
        }
        if (start > 0) {
            g_dataBuffer.erase(0, start);
        }

        if (g_dataBuffer.rfind(zowi::kMessagePrefix, 0) == 0) {
            auto tokenEnd = g_dataBuffer.find(zowi::kMessageTerminator);
            if (tokenEnd == std::string::npos) break;

            std::string token = g_dataBuffer.substr(0, tokenEnd);
            g_dataBuffer.erase(0, tokenEnd + (sizeof(zowi::kMessageTerminator) - 1));
            parseRobotMessageUnlocked(token);
            continue;
        }

        auto lineEnd = g_dataBuffer.find('\n');
        if (lineEnd == std::string::npos) break;

        std::string line = g_dataBuffer.substr(0, lineEnd);
        g_dataBuffer.erase(0, lineEnd + 1);
        parseRobotMessageUnlocked(line);
    }
}

static bool waitUntil(QCoreApplication &qtApp, int timeoutMs, const std::function<bool()> &predicate)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        qtApp.processEvents();
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

// ── Interactive keyboard input (raw terminal mode) ───────────
// The minigame reads the arrow keys directly from stdin. We switch the
// terminal to raw mode (no line buffering, no echo) and decode the ANSI
// escape sequences the cursor keys produce:
//   ↑ = ESC [ A   ↓ = ESC [ B   → = ESC [ C   ← = ESC [ D
// (Some terminals emit ESC O A/B/C/D instead of ESC [ A/B/C/D.)
static int g_stdinFd = STDIN_FILENO;
static struct termios g_origTermios;
static bool g_rawMode = false;

static bool enableRawMode()
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

static void disableRawMode()
{
    if (g_rawMode) {
        tcsetattr(g_stdinFd, TCSAFLUSH, &g_origTermios);
        g_rawMode = false;
    }
}

// Returns a logical key token, or "" if no (complete) key is available.
static std::string readKey()
{
    unsigned char c;
    ssize_t n = read(g_stdinFd, &c, 1);
    if (n <= 0) return "";

    if (c == 3) return "quit";          // Ctrl-C
    if (c != 0x1b) {                    // non-escape: only 'q' quits
        if (c == 'q' || c == 'Q') return "quit";
        return "";
    }

    // Escape sequence: read the rest with a short timeout.
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
    return "";
}

static bool waitForRobotData(QCoreApplication &qtApp, int timeoutMs)
{
    return waitUntil(qtApp, timeoutMs, []() {
        std::lock_guard<std::mutex> lock(g_mtx);
        return !g_robotName.empty() && !g_appId.empty() && g_battery >= 0;
    });
}

static bool waitForBatteryLevel(QCoreApplication &qtApp, int timeoutMs)
{
    return waitUntil(qtApp, timeoutMs, []() {
        std::lock_guard<std::mutex> lock(g_mtx);
        return g_battery >= 0;
    });
}

static bool waitForAppId(QCoreApplication &qtApp, zowi::BluetoothApi &bt, int timeoutMs,
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

// ── Firmware upload (delegates to the zowi_firmware library) ──
// protocol: "raw" streams the Intel HEX file to the device's custom bootloader;
//           "stk" uses the STK500v1 / Optiboot framing.
static bool uploadFirmware(zowi::BluetoothApi &bt,
                            QCoreApplication &qtApp,
                            const std::string &firmwarePath,
                            const std::string &protocol)
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
        std::cerr << "[debug] bytes received during upload (" << g_stkBuffer.size() << "): ";
        for (char c : g_stkBuffer) {
            std::cerr << "0x" << std::hex << (static_cast<int>(c) & 0xFF) << std::dec << " ";
        }
        std::cerr << std::endl;
        g_uploadMode = false;
    }

    return ok;
}

// Prepare a serial (RFCOMM TTY) backend for flashing. If ttyOpt is empty, the
// function binds an RFCOMM TTY to the paired device address (requires
// CAP_NET_ADMIN/root) and returns the bound device path in boundTty so the
// caller can release it afterwards. The returned backend is NOT yet connected;
// the caller sets callbacks and calls connect(tty) (which pulses DTR).
static std::unique_ptr<zowi::SerialBluetoothBackend> prepareFlashBackend(
    const std::string &address,
    const std::string &ttyOpt,
    std::string &tty,
    std::string &boundTty)
{
    tty = ttyOpt;
    if (tty.empty()) {
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
    return std::make_unique<zowi::SerialBluetoothBackend>();
}

static bool installFirmwareToPairedZowi(QCoreApplication &qtApp,
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

    // Note: the backend is expected to be already connected by the caller
    // (the flashing flow opens an RFCOMM TTY and pulses DTR to reset the robot
    // into its bootloader before calling this function). The paired device
    // address is supplied via --address / the session when available.

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

int main(int argc, char **argv)
{
    CLI::App app{"Zowi Desktop CLI"};

    // ── session subcommand ────────────────────────────────────
    auto *sessionCmd = app.add_subcommand("session", "Manage session data");
    auto *getSession = sessionCmd->add_subcommand("get", "Get a session value");
    auto *setSession = sessionCmd->add_subcommand("set", "Set a session value");
    auto *listSession = sessionCmd->add_subcommand("list", "List all session keys and values");

    std::string getKey;
    getSession->add_option("key", getKey, "Key to read")->required();

    std::string setKey, setValue;
    setSession->add_option("key", setKey, "Key to write")->required();
    setSession->add_option("value", setValue, "Value to write")->required();

    // ── translate subcommand ──────────────────────────────────
    auto *translateCmd = app.add_subcommand("translate", "Translate a string");
    std::string trLocale, trContext, trSource;
    translateCmd->add_option("--locale,-l", trLocale, "Locale (es_ES, ca_ES, en_US)")->default_val("es_ES");
    translateCmd->add_option("--context,-c", trContext, "Context (e.g. WelcomeScreen.qml)")->default_val("");
    translateCmd->add_option("--source,-s", trSource, "Source text to translate")->required();

    // ── config subcommand ─────────────────────────────────────
    auto *configCmd = app.add_subcommand("config", "Read config values");
    auto *configGet = configCmd->add_subcommand("get", "Get a config value");
    auto *configList = configCmd->add_subcommand("list", "List all config keys and values");

    std::string configKey;
    configGet->add_option("key", configKey, "Config key to read")->required();

    // ── scan subcommand ───────────────────────────────────────
    auto *scanCmd = app.add_subcommand("scan", "Scan for nearby Zowi robots (5s)\nFilters by name and MAC prefix by default.\nRun with sudo to avoid CAP_NET_ADMIN warning.");
    int scanTimeout = 5;
    bool filterName = true;
    bool filterMac = true;
    scanCmd->add_option("--timeout,-t", scanTimeout, "Scan duration in seconds")->default_val(5);
    scanCmd->add_flag("--no-filter-name", filterName, "Disable filtering by Zowi name");
    scanCmd->add_flag("--no-filter-mac", filterMac, "Disable filtering by MAC prefix");

    // ── connect subcommand ────────────────────────────────────
    auto *connectCmd = app.add_subcommand("connect", "Connect to a Zowi robot by Bluetooth address\nReceives robot name, app ID, and battery level.\nRun with sudo to avoid CAP_NET_ADMIN warning.");
    std::string connectAddress;
    connectCmd->add_option("address", connectAddress, "Bluetooth MAC address (e.g. B4:9D:0B:32:41:0E)")->required();
    int connectTimeout = 3;
    connectCmd->add_option("--timeout,-t", connectTimeout, "Timeout waiting for robot data (seconds)")->default_val(3);

    // ── rename subcommand ─────────────────────────────────────
    auto *renameCmd = app.add_subcommand("rename", "Rename the connected Zowi robot\nConnects to the saved device, sends the rename command, and saves the new name.");
    std::string newName;
    renameCmd->add_option("name", newName, "New name for the robot")->required();
    renameCmd->add_option("--timeout,-t", connectTimeout, "Timeout waiting for robot response (seconds)")->default_val(3);

    // ── disconnect subcommand ─────────────────────────────────
    auto *disconnectCmd = app.add_subcommand("disconnect", "Disconnect and clear saved pairing data");

    // ── status subcommand ─────────────────────────────────────
    auto *statusCmd = app.add_subcommand("status", "Show current Zowi connection status");

    // ── restore subcommand ────────────────────────────────────
    auto *restoreCmd = app.add_subcommand("restore", "Restore the original factory firmware to the paired Zowi robot\nUploads the bundled ZOWI_BASE_v2.hex file unless a custom path is provided.");
    std::string restoreFirmwarePath = kFactoryFirmwarePath;
    int restoreTimeout = 10;
    int restoreBatteryTimeout = 2;
    bool forceLowBattery = false;
    restoreCmd->add_option("--firmware,-f", restoreFirmwarePath, "Path to the firmware .hex file to upload")->default_val(kFactoryFirmwarePath);
    restoreCmd->add_option("--timeout,-t", restoreTimeout, "Seconds to wait for firmware restore confirmation")->default_val(10);
    restoreCmd->add_option("--battery-timeout", restoreBatteryTimeout, "Seconds to wait for a battery reading before uploading")->default_val(2);
    restoreCmd->add_flag("--force-low-battery", forceLowBattery, "Continue even if the reported battery level is below 50%");
    std::string restoreProtocol = "raw";
    restoreCmd->add_option("--protocol", restoreProtocol, "Upload protocol: 'raw' (stream HEX to custom bootloader) or 'stk' (STK500v1)")->default_val("stk");
    std::string restoreTty;
    restoreCmd->add_option("--tty", restoreTty, "RFCOMM TTY to use for flashing (e.g. /dev/rfcomm0). If omitted, one is bound automatically (needs root).")->default_val("");
    std::string restoreAddress;
    restoreCmd->add_option("--address,-a", restoreAddress, "Robot Bluetooth address (overrides the paired device from the session)")->default_val("");

    // ── alarm subcommand ──────────────────────────────────────
    auto *alarmCmd = app.add_subcommand("alarm", "Install the Robot Alarm firmware on the paired Zowi robot\nUploads the bundled ZOWI_Alarm_v2.hex file unless a custom path is provided.");
    std::string alarmFirmwarePath = kAlarmFirmwarePath;
    int alarmTimeout = 10;
    int alarmBatteryTimeout = 2;
    bool forceAlarmLowBattery = false;
    alarmCmd->add_option("--firmware,-f", alarmFirmwarePath, "Path to the alarm firmware .hex file to upload")->default_val(kAlarmFirmwarePath);
    alarmCmd->add_option("--timeout,-t", alarmTimeout, "Seconds to wait for alarm firmware confirmation")->default_val(10);
    alarmCmd->add_option("--battery-timeout", alarmBatteryTimeout, "Seconds to wait for a battery reading before uploading")->default_val(2);
    alarmCmd->add_flag("--force-low-battery", forceAlarmLowBattery, "Continue even if the reported battery level is below 50%");
    std::string alarmProtocol = "raw";
    alarmCmd->add_option("--protocol", alarmProtocol, "Upload protocol: 'raw' (stream HEX to custom bootloader) or 'stk' (STK500v1)")->default_val("stk");
    std::string alarmTty;
    alarmCmd->add_option("--tty", alarmTty, "RFCOMM TTY to use for flashing (e.g. /dev/rfcomm0). If omitted, one is bound automatically (needs root).")->default_val("");
    std::string alarmAddress;
    alarmCmd->add_option("--address,-a", alarmAddress, "Robot Bluetooth address (overrides the paired device from the session)")->default_val("");

    // ── control subcommand ───────────────────────────────────
    auto *controlCmd = app.add_subcommand("control", "Interactive keyboard minigame to drive the Zowi robot\nUse the arrow keys to move; press ESC or 'q' to quit.\nConnects to the paired device (or --address).");
    std::string controlAddress;
    controlCmd->add_option("--address,-a", controlAddress, "Robot Bluetooth address (overrides the paired device from the session)")->default_val("");
    std::string controlSpeed = "medium";
    controlCmd->add_option("--speed", controlSpeed, "Movement speed: slow, medium, fast")->default_val("medium");
    int controlTimeout = 3;
    controlCmd->add_option("--timeout,-t", controlTimeout, "Timeout waiting for connection (seconds)")->default_val(3);

    CLI11_PARSE(app, argc, argv);

    // ── session ───────────────────────────────────────────────
    if (*sessionCmd) {
        zowi::SessionStore store("ZowiDesktop", "ZowiApp");

        if (*getSession) {
            if (store.contains(getKey)) {
                std::cout << store.getRaw(getKey) << std::endl;
            } else {
                std::cerr << "Key not found: " << getKey << std::endl;
                return 1;
            }
        } else if (*setSession) {
            if (setValue == "true" || setValue == "false") {
                store.setBool(setKey, setValue == "true");
            } else {
                try {
                    int intVal = std::stoi(setValue);
                    store.setInt(setKey, intVal);
                } catch (...) {
                    store.setString(setKey, setValue);
                }
            }
            std::cout << "OK" << std::endl;
        } else if (*listSession) {
            for (const auto &k : store.keys()) {
                std::cout << k << "=" << store.getRaw(k) << std::endl;
            }
        }
    }

    // ── translate ─────────────────────────────────────────────
    if (*translateCmd) {
        zowi::TranslationEngine engine;
        std::string basePath = ".";
        engine.setResourceBasePath(basePath);
        engine.load(trLocale);
        std::string result = engine.translate(trContext, trSource);
        std::cout << result << std::endl;
    }

    // ── config ────────────────────────────────────────────────
    if (*configCmd) {
        zowi::ConfigStore store("src/config.json");

        if (*configGet) {
            std::string val = store.get(configKey);
            if (val.empty()) {
                std::cerr << "Key not found: " << configKey << std::endl;
                return 1;
            }
            std::cout << val << std::endl;
        } else if (*configList) {
            for (const auto &k : store.keys()) {
                std::string val = store.get(k);
                std::cout << k << "=" << val << std::endl;
            }
        }
    }

    // ── scan ──────────────────────────────────────────────────
    if (*scanCmd) {
        QCoreApplication qtApp(argc, argv);
        zowi::QtBluetoothBackend bt;

        zowi::ConfigStore config("src/config.json");
        std::string macPrefix = config.get("zowi_mac_prefix");

        std::atomic<int> deviceCount{0};

        bt.onDeviceFound([&](const zowi::DeviceInfo &dev) {
            if (filterName) {
                std::string nameLower = dev.name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                if (nameLower.find("zowi") == std::string::npos) {
                    if (filterMac && !macPrefix.empty()) {
                        std::string addrUpper = dev.address;
                        std::transform(addrUpper.begin(), addrUpper.end(), addrUpper.begin(), ::toupper);
                        std::string prefixUpper = macPrefix;
                        std::transform(prefixUpper.begin(), prefixUpper.end(), prefixUpper.begin(), ::toupper);
                        if (addrUpper.find(prefixUpper) != 0)
                            return;
                    } else {
                        return;
                    }
                }
            } else if (filterMac && !macPrefix.empty()) {
                std::string addrUpper = dev.address;
                std::transform(addrUpper.begin(), addrUpper.end(), addrUpper.begin(), ::toupper);
                std::string prefixUpper = macPrefix;
                std::transform(prefixUpper.begin(), prefixUpper.end(), prefixUpper.begin(), ::toupper);
                if (addrUpper.find(prefixUpper) != 0)
                    return;
            }

            deviceCount++;
            std::cout << dev.name << " [" << dev.address << "]" << std::endl;
        });

        bt.onError([&](const std::string &msg) {
            std::cerr << "Error: " << msg << std::endl;
        });

        std::cout << "Scanning for " << scanTimeout << "s..." << std::endl;
        bt.startDiscovery();

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(scanTimeout);
        while (std::chrono::steady_clock::now() < deadline) {
            qtApp.processEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        bt.stopDiscovery();

        if (deviceCount == 0) {
            std::cout << "No devices found." << std::endl;
        } else {
            std::cout << deviceCount << " device(s) found." << std::endl;
        }
    }

    // ── connect ───────────────────────────────────────────────
    if (*connectCmd) {
        QCoreApplication qtApp(argc, argv);
        zowi::QtBluetoothBackend bt;
        resetRobotState();

        std::cout << "Connecting to " << connectAddress << "..." << std::endl;

        bt.onDataReceived([](const std::string &data) {
            onDataReceived(data);
        });

        bt.onConnectionChanged([&](bool connected) {
            g_connected = connected;
            if (connected) {
                std::cout << "Connected. Waiting for robot data..." << std::endl;
            } else {
                std::cout << "Disconnected." << std::endl;
            }
        });

        bt.onError([&](const std::string &msg) {
            std::cerr << "Error: " << msg << std::endl;
        });

        bt.connect(connectAddress);

        // Process events while waiting for connection + data
        waitForRobotData(qtApp, (connectTimeout + 2) * 1000);

        bt.disconnect();

        // Display results
        std::cout << std::endl;
        if (!g_robotName.empty()) {
            std::cout << "  Name:    " << g_robotName << std::endl;
        } else {
            std::cout << "  Name:    (not received)" << std::endl;
        }
        if (!g_appId.empty()) {
            std::cout << "  App ID:  " << g_appId << std::endl;
        } else {
            std::cout << "  App ID:  (not received)" << std::endl;
        }
        if (g_battery >= 0) {
            std::cout << "  Battery: " << g_battery << "%" << std::endl;
        } else {
            std::cout << "  Battery: (not received)" << std::endl;
        }
        std::cout << "  Address: " << connectAddress << std::endl;

        // Save to session
        zowi::SessionStore store("ZowiDesktop", "ZowiApp");
        store.setString("activeZowiDeviceAddress", connectAddress);
        if (!g_robotName.empty()) {
            store.setString("activeZowiName", g_robotName);
        }
        if (!g_appId.empty()) {
            store.setString("activeZowiAppId", g_appId);
        }
        if (g_battery >= 0) {
            store.setInt("activeZowiBattery", static_cast<int>(g_battery));
        }
        store.setBool("wizardDismissed", true);

        std::cout << std::endl << "Pairing saved to session." << std::endl;
    }

    // ── rename ────────────────────────────────────────────────
    if (*renameCmd) {
        QCoreApplication qtApp(argc, argv);
        zowi::QtBluetoothBackend bt;
        resetRobotState();

        // Load saved address
        zowi::SessionStore session("ZowiDesktop", "ZowiApp");
        std::string savedAddr = session.getString("activeZowiDeviceAddress");
        if (savedAddr.empty()) {
            std::cerr << "No paired device found. Run 'connect' first." << std::endl;
            return 1;
        }

        std::cout << "Connecting to " << savedAddr << "..." << std::endl;

        std::atomic<bool> renameSent{false};

        bt.onConnectionChanged([&](bool connected) {
            g_connected = connected;
            if (connected) {
                std::cout << "Connected. Sending rename command..." << std::endl;
                const std::string cmd = zowi::makeCommand(zowi::Command::SetName, newName);
                bt.send(cmd);
                renameSent = true;
            }
        });

        bt.onDataReceived([&](const std::string &data) {
            onDataReceived(data);
        });

        bt.onError([&](const std::string &msg) {
            std::cerr << "Error: " << msg << std::endl;
        });

        bt.connect(savedAddr);

        // Wait for the robot to acknowledge the rename with a final ack (&&F),
        // which the firmware sends only AFTER writing the new name to EEPROM.
        // (g_dataReceived is already true from the identity messages on connect,
        // so it alone would let us disconnect before the write completes.)
        bool renamed = false;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(connectTimeout + 2);
        while (std::chrono::steady_clock::now() < deadline) {
            qtApp.processEvents();
            {
                std::lock_guard<std::mutex> lock(g_mtx);
                if (renameSent && g_finalAck) { renamed = true; break; }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (!renamed) {
            std::cerr << "Warning: the robot did not acknowledge the rename; the new name may not have been saved." << std::endl;
        }

        // Give the firmware a moment to finish the EEPROM write before we drop
        // the connection.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        bt.disconnect();

        // Save new name
        session.setString("activeZowiName", newName);
        std::cout << "Robot renamed to '" << newName << "'." << std::endl;
        std::cout << "Session updated." << std::endl;
    }

    // ── restore ───────────────────────────────────────────────
    if (*restoreCmd) {
        QCoreApplication qtApp(argc, argv);
        zowi::SessionStore session("ZowiDesktop", "ZowiApp");
        std::string tty, boundTty;
        const std::string restoreAddr = restoreAddress.empty()
                                            ? session.getString("activeZowiDeviceAddress")
                                            : restoreAddress;
        auto bt = prepareFlashBackend(restoreAddr, restoreTty, tty, boundTty);
        if (!bt) return 1;
        bt->onDataReceived([](const std::string &d) { onDataReceived(d); });
        bt->onConnectionChanged([](bool c) {
            g_connected = c;
            std::cout << (c ? "Serial connection open." : "Disconnected.") << std::endl;
        });
        bt->onError([](const std::string &m) { std::cerr << "Error: " << m << std::endl; });
        if (!bt->connect(tty)) {
            std::cerr << "Failed to open serial backend: " << bt->lastError() << std::endl;
            if (!boundTty.empty()) std::system("rfcomm release 0");
            return 1;
        }
        const bool ok = installFirmwareToPairedZowi(qtApp,
                                                    *bt,
                                                    session,
                                                    "Factory firmware restore",
                                                    restoreFirmwarePath,
                                                    restoreBatteryTimeout,
                                                    restoreTimeout,
                                                    forceLowBattery,
                                                    restoreProtocol);
        if (!boundTty.empty()) std::system("rfcomm release 0");
        if (!ok) return 1;
    }

    // ── alarm ─────────────────────────────────────────────────
    if (*alarmCmd) {
        QCoreApplication qtApp(argc, argv);
        zowi::SessionStore session("ZowiDesktop", "ZowiApp");
        std::string tty, boundTty;
        const std::string alarmAddr = alarmAddress.empty()
                                          ? session.getString("activeZowiDeviceAddress")
                                          : alarmAddress;
        auto bt = prepareFlashBackend(alarmAddr, alarmTty, tty, boundTty);
        if (!bt) return 1;
        bt->onDataReceived([](const std::string &d) { onDataReceived(d); });
        bt->onConnectionChanged([](bool c) {
            g_connected = c;
            std::cout << (c ? "Serial connection open." : "Disconnected.") << std::endl;
        });
        bt->onError([](const std::string &m) { std::cerr << "Error: " << m << std::endl; });
        if (!bt->connect(tty)) {
            std::cerr << "Failed to open serial backend: " << bt->lastError() << std::endl;
            if (!boundTty.empty()) std::system("rfcomm release 0");
            return 1;
        }
        const bool ok = installFirmwareToPairedZowi(qtApp,
                                                    *bt,
                                                    session,
                                                    "Alarm firmware installation",
                                                    alarmFirmwarePath,
                                                    alarmBatteryTimeout,
                                                    alarmTimeout,
                                                    forceAlarmLowBattery,
                                                    alarmProtocol);
        if (!boundTty.empty()) std::system("rfcomm release 0");
        if (!ok) return 1;
    }

    // ── disconnect ────────────────────────────────────────────
    if (*disconnectCmd) {
        zowi::SessionStore store("ZowiDesktop", "ZowiApp");

        std::string savedName = store.getString("activeZowiName");
        std::string savedAddr = store.getString("activeZowiDeviceAddress");

        store.setString("activeZowiDeviceAddress", "");
        store.setString("activeZowiName", "");
        store.setBool("wizardDismissed", false);

        if (!savedAddr.empty()) {
            std::string label = savedName.empty() ? "(unknown)" : savedName;
            std::cout << "Disconnected from " << label
                      << " [" << savedAddr << "]" << std::endl;
        } else {
            std::cout << "No device was paired." << std::endl;
        }
        std::cout << "Pairing data cleared." << std::endl;
    }

    // ── status ────────────────────────────────────────────────
    if (*statusCmd) {
        zowi::SessionStore store("ZowiDesktop", "ZowiApp");

        std::string addr = store.getString("activeZowiDeviceAddress");
        std::string name = store.getString("activeZowiName");
        std::string appId = store.getString("activeZowiAppId");
        int battery = store.getInt("activeZowiBattery", -1);
        bool dismissed = store.getBool("wizardDismissed", false);

        if (addr.empty()) {
            std::cout << "No Zowi connected." << std::endl;
            return 0;
        }

        // Attempt a live connection so the reported state reflects the robot's
        // actual running firmware rather than the (possibly stale) session cache.
        bool live = false;
        {
            QCoreApplication qtApp(argc, argv);
            zowi::QtBluetoothBackend bt;
            resetRobotState();

            bt.onDataReceived([](const std::string &data) { onDataReceived(data); });
            bt.onConnectionChanged([](bool connected) {
                if (connected) std::cout << "Connected. Reading live status..." << std::endl;
            });
            bt.onError([](const std::string &msg) { std::cerr << "Error: " << msg << std::endl; });

            std::cout << "Connecting to " << addr << "..." << std::endl;
            bt.connect(addr);
            if (waitForRobotData(qtApp, (connectTimeout + 2) * 1000)) {
                live = true;
                if (!g_robotName.empty()) name = g_robotName;
                if (!g_appId.empty()) appId = g_appId;
                if (g_battery >= 0) battery = static_cast<int>(g_battery);

                // Refresh the session cache so future reads stay consistent.
                store.setString("activeZowiName", name);
                store.setString("activeZowiAppId", appId);
                store.setInt("activeZowiBattery", battery);
            }
            bt.disconnect();
        }

        if (!live) {
            std::cout << "Could not reach the robot; showing last known (cached) state." << std::endl;
        }

        std::cout << "Zowi connected:" << std::endl;
        std::cout << "  Name:    " << (name.empty() ? "(unknown)" : name) << std::endl;
        std::cout << "  Address: " << addr << std::endl;
        std::cout << "  App ID:  " << (appId.empty() ? "(unknown)" : appId)
                  << (live ? "" : "  (cached)") << std::endl;
        std::cout << "  Battery: " << (battery >= 0 ? std::to_string(battery) + "%" : "(unknown)")
                  << (live ? "" : "  (cached)") << std::endl;
        std::cout << "  Wizard:  " << (dismissed ? "completed" : "not completed") << std::endl;
    }

    // ── control ───────────────────────────────────────────────
    if (*controlCmd) {
        QCoreApplication qtApp(argc, argv);
        zowi::QtBluetoothBackend bt;
        resetRobotState();

        zowi::MovementSpeed speed = zowi::MovementSpeed::Medium;
        if (controlSpeed == "slow") speed = zowi::MovementSpeed::Slow;
        else if (controlSpeed == "fast") speed = zowi::MovementSpeed::Fast;
        else if (controlSpeed != "medium") {
            std::cerr << "Unknown speed '" << controlSpeed << "'; using 'medium'.\n";
        }

        zowi::SessionStore session("ZowiDesktop", "ZowiApp");
        const std::string ctrlAddr = controlAddress.empty()
                                         ? session.getString("activeZowiDeviceAddress")
                                         : controlAddress;
        if (ctrlAddr.empty()) {
            std::cerr << "No paired device found. Run 'connect' first or pass --address." << std::endl;
            return 1;
        }

        std::cout << "Connecting to " << ctrlAddr << "..." << std::endl;

        bt.onDataReceived([](const std::string &data) { onDataReceived(data); });
        bt.onConnectionChanged([&](bool connected) {
            g_connected = connected;
            if (connected) {
                std::cout << "Connected. Drive with the arrow keys (ESC or 'q' to quit)." << std::endl;
            } else {
                std::cout << "Disconnected." << std::endl;
            }
        });
        bt.onError([&](const std::string &msg) { std::cerr << "Error: " << msg << std::endl; });

        bt.connect(ctrlAddr);

        if (!waitUntil(qtApp, controlTimeout * 1000, []() {
                std::lock_guard<std::mutex> lock(g_mtx);
                return g_connected;
            })) {
            std::cerr << "Could not connect to the robot within " << controlTimeout << "s." << std::endl;
            bt.disconnect();
            return 1;
        }

        // Non-blocking battery warning.
        if (waitForBatteryLevel(qtApp, 2000) && g_battery >= 0 && g_battery < kLowBatteryThreshold) {
            std::cout << "Warning: battery is low (" << g_battery << "%). Consider recharging.\n";
        }

        const bool interactive = isatty(g_stdinFd) && enableRawMode();
        if (interactive) {
            std::atexit(disableRawMode);
            std::signal(SIGINT, [](int) {
                g_quit.store(true);
                disableRawMode();
            });
        } else {
            std::cout << "Interactive control requires a terminal; cannot read arrow keys.\n";
            bt.send(zowi::commandStop());
            bt.disconnect();
            return 0;
        }

        std::cout << "Controls:  UP=forward  DOWN=backward  LEFT=turn left  RIGHT=turn right  (ESC/q=quit)\n";

        while (!g_quit.load()) {
            {
                std::lock_guard<std::mutex> lock(g_mtx);
                if (!g_connected) break;
            }
            qtApp.processEvents();

            std::string key = readKey();
            if (!key.empty()) {
                if (key == "quit") {
                    g_quit.store(true);
                    break;
                }
                std::string cmd;
                std::string action;
                if (key == "up") { cmd = zowi::commandWalkForward(speed); action = "forward"; }
                else if (key == "down") { cmd = zowi::commandWalkBackward(speed); action = "backward"; }
                else if (key == "left") { cmd = zowi::commandTurnLeft(speed); action = "turn left"; }
                else if (key == "right") { cmd = zowi::commandTurnRight(speed); action = "turn right"; }

                if (!cmd.empty()) {
                    bt.send(cmd);
                    std::cout << "\r" << action << "          " << std::flush;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        // Stop the robot and restore the terminal before leaving.
        bt.send(zowi::commandStop());
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        disableRawMode();
        bt.disconnect();
        std::cout << "\nStopped. Disconnected. Bye!\n";
    }

    return 0;
}
