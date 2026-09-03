#include "cli_commands.h"
#include "cli_state.h"
#include "cli_util.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <array>
#include <csignal>
#include <chrono>
#include <thread>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#define isatty _isatty
#endif

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

#include <zowi/session_store.h>
#include <zowi/transport_constants.h>
#include <zowi/translation_engine.h>
#include <zowi/config_store.h>
#include <zowi/robot_commands.h>
#include <zowi/protocol.h>
#include <zowi/calibration_session.h>
#ifndef _WIN32
#include <qt_bluetooth_backend.h>
#endif
#ifdef ZOWI_HAVE_SERIAL
#ifdef _WIN32
#include <win_serial_backend.h>
using SerialBackend = zowi::WinSerialBackend;
#else
#include <serial_bluetooth_backend.h>
using SerialBackend = zowi::SerialBluetoothBackend;
#endif
#endif

namespace zowi_cli {

int runSession(const SessionArgs &a)
{
    zowi::SessionStore store("ZowiDesktop", "ZowiApp");

    if (a.get) {
        if (store.contains(a.getKey)) {
            std::cout << store.getRaw(a.getKey) << std::endl;
        } else {
            std::cerr << "Key not found: " << a.getKey << std::endl;
            return 1;
        }
    } else if (a.set) {
        if (a.setValue == "true" || a.setValue == "false") {
            store.setBool(a.setKey, a.setValue == "true");
        } else {
            try {
                int intVal = std::stoi(a.setValue);
                store.setInt(a.setKey, intVal);
            } catch (...) {
                store.setString(a.setKey, a.setValue);
            }
        }
        std::cout << "OK" << std::endl;
    } else if (a.list) {
        for (const auto &k : store.keys()) {
            std::cout << k << "=" << store.getRaw(k) << std::endl;
        }
    } else if (a.clear) {
        for (const auto &k : store.keys()) {
            store.removeKey(k);
        }
        std::cout << "Session cleared." << std::endl;
    }
    return 0;
}

int runTranslate(const TranslateArgs &a)
{
    zowi::TranslationEngine engine;
    engine.setResourceBasePath(".");
    engine.load(a.locale);
    std::string result = engine.translate(a.context, a.source);
    std::cout << result << std::endl;
    return 0;
}

int runConfig(const ConfigArgs &a)
{
    zowi::ConfigStore store("src/config.json");

    if (a.get) {
        std::string val = store.get(a.key);
        if (val.empty()) {
            std::cerr << "Key not found: " << a.key << std::endl;
            return 1;
        }
        std::cout << val << std::endl;
    } else if (a.list) {
        for (const auto &k : store.keys()) {
            std::string val = store.get(k);
            std::cout << k << "=" << val << std::endl;
        }
    }
    return 0;
}

#ifdef ZOWI_HAVE_SERIAL
int runPorts()
{
    auto ports = SerialBackend::listSerialPorts();
    if (ports.empty()) {
        std::cout << "No USB serial ports found (/dev/ttyUSB*, /dev/ttyACM*)." << std::endl;
    } else {
        std::cout << "Available USB serial ports:" << std::endl;
        for (const auto &p : ports) std::cout << "  " << p << std::endl;
    }
    return 0;
}
#else
int runPorts()
{
    std::cout << "USB serial ports not available on this platform." << std::endl;
    return 0;
}
#endif

int runScan(int argc, char **argv, const ScanArgs &a)
{
    QCoreApplication qtApp(argc, argv);
#ifdef ZOWI_HAVE_NATIVE_BT
    zowi::NativeBluetoothBackend bt;
#else
    zowi::QtBluetoothBackend bt;
#endif

    zowi::ConfigStore config("src/config.json");
    std::string macPrefix = config.get("zowi_mac_prefix");

    std::atomic<int> deviceCount{0};

    bt.onDeviceFound([&](const zowi::DeviceInfo &dev) {
        if (a.filterName) {
            std::string nameLower = dev.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find("zowi") == std::string::npos) {
                if (a.filterMac && !macPrefix.empty()) {
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
        } else if (a.filterMac && !macPrefix.empty()) {
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

    std::cout << "Scanning for " << a.timeout << "s..." << std::endl;
    bt.startDiscovery();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(a.timeout);
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
    return 0;
}

int runConnect(int argc, char **argv, const ConnectArgs &a)
{
    QCoreApplication qtApp(argc, argv);
    resetRobotState();

    std::string target, boundTty;
    auto bt = prepareFlashBackend(a.backend, a.address, a.tty, a.baud, target, boundTty);
    if (!bt) return 1;

    // Over USB the robot takes a while to boot and start emitting data, so give
    // it a longer default timeout (8s) unless the user asked for more.
#ifdef ZOWI_HAVE_SERIAL
    const bool isUsb = (dynamic_cast<SerialBackend *>(bt.get()) != nullptr);
#else
    const bool isUsb = false;
#endif
    const int effTimeout = isUsb ? std::max(a.timeout, 8) : a.timeout;

#ifndef ZOWI_HAVE_NATIVE_BT
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << target << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, target, kDiscoveryTimeoutMs);
    }
#endif

    std::cout << "Connecting to " << target << "..." << std::endl;

    bt->onDataReceived([](const std::string &data) {
        onDataReceived(data);
    });

    bt->onConnectionChanged([&](bool connected) {
        g_connected = connected;
        if (connected) {
            std::cout << "Connected. Waiting for robot data..." << std::endl;
        } else {
            std::cout << "\n\nDisconnected." << std::endl;
        }
    });

    bt->onError([&](const std::string &msg) {
        std::cerr << "Error: " << msg << std::endl;
    });

    bt->connect(target);

    // Poll until the robot reports its full identity (it reboots right after
    // the link opens, so the first request is often swallowed).
    waitForRobotIdentity(qtApp, *bt, (effTimeout + 2) * 1000);

    bt->disconnect();
    if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");

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
    std::cout << "  Address: " << target << std::endl;

    zowi::SessionStore store("ZowiDesktop", "ZowiApp");
    store.setString("activeZowiDeviceAddress", target);
    store.setString("activeZowiTransport", a.backend == "usb" ? zowi::kTransportUsb : zowi::kTransportBt);
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
    return 0;
}

int runRename(int argc, char **argv, const RenameArgs &a)
{
    QCoreApplication qtApp(argc, argv);
    resetRobotState();

    zowi::SessionStore session("ZowiDesktop", "ZowiApp");
    std::string savedAddr = session.getString("activeZowiDeviceAddress");
    if (savedAddr.empty()) {
        std::cerr << "No paired device found. Run 'connect' first." << std::endl;
        return 1;
    }

    // Pick the backend: explicit --backend, else the transport used at connect
    // time, else Bluetooth by default.
    std::string backend = a.backend;
    if (backend == "auto") {
        backend = session.getString("activeZowiTransport");
        if (backend == "bt") backend = "bluetooth";
        if (backend.empty()) backend = "bluetooth";
    }

    std::string target, boundTty;
    auto bt = prepareFlashBackend(backend, savedAddr, a.tty, a.baud, target, boundTty);
    if (!bt) return 1;

#ifdef ZOWI_HAVE_SERIAL
    const bool isUsb = (dynamic_cast<SerialBackend *>(bt.get()) != nullptr);
#else
    const bool isUsb = false;
#endif
    const int effTimeout = isUsb ? std::max(a.timeout, 8) : a.timeout;

#ifndef ZOWI_HAVE_NATIVE_BT
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << target << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, target, kDiscoveryTimeoutMs);
    }
#endif

    std::cout << "Connecting to " << target << "..." << std::endl;

    std::atomic<bool> renameSent{false};

    bt->onConnectionChanged([&](bool connected) {
        g_connected = connected;
    });

    bt->onDataReceived([&](const std::string &data) {
        onDataReceived(data);
    });

    bt->onError([&](const std::string &msg) {
        std::cerr << "Error: " << msg << std::endl;
    });

    bt->connect(target);
    requestRobotData(*bt);

    bool renamed = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(effTimeout + 2);
    while (std::chrono::steady_clock::now() < deadline) {
        qtApp.processEvents();
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            if (g_connected && g_dataReceived && !renameSent) {
                std::cout << "Connected. Sending rename command..." << std::endl;
                const std::string cmd = zowi::makeCommand(zowi::Command::SetName, a.name);
                if (g_debugLog) {
                    std::cout << "robot tx: R " << a.name << std::endl;
                }
                bt->send(cmd);
                renameSent = true;
            }
            if (renameSent && g_finalAck) { renamed = true; break; }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!renamed) {
        std::cerr << "Warning: the robot did not acknowledge the rename; the new name may not have been saved." << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    bt->disconnect();
    if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");

    session.setString("activeZowiName", a.name);
    std::cout << "Robot renamed to '" << a.name << "'." << std::endl;
    std::cout << "Session updated." << std::endl;
    return 0;
}

// Shared implementation for restore / alarm / adivinawi. Prepares the backend,
// connects, installs the firmware and releases any bound RFCOMM TTY.
int runFirmware(int argc, char **argv, const FirmwareArgs &a, const std::string &actionLabel)
{
    QCoreApplication qtApp(argc, argv);
    zowi::SessionStore session("ZowiDesktop", "ZowiApp");
    std::string connectTarget, boundTty;
    const std::string addr = a.address.empty()
                                 ? session.getString("activeZowiDeviceAddress")
                                 : a.address;
    auto bt = prepareFlashBackend(a.backend, addr, a.tty, a.baud, connectTarget, boundTty);
    if (!bt) return 1;
    // Firmware flashing drives the bootloader explicitly via pulseReset(); do
    // not add the control-connection boot delay.
#ifdef ZOWI_HAVE_SERIAL
    if (auto *serial = dynamic_cast<SerialBackend *>(bt.get()))
        serial->setBootDelayMs(0);
#endif
#ifndef ZOWI_HAVE_NATIVE_BT
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << connectTarget << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, connectTarget, kDiscoveryTimeoutMs);
    }
#endif
    bt->setAutoReconnect(true, 100);
    bt->onDataReceived([](const std::string &d) { onDataReceived(d); });
    bt->onConnectionChanged([](bool c) {
        g_connected = c;
        if (c) g_connectedOnce = true;
        std::cout << (c ? "Connection open." : "Disconnected.") << std::endl;
    });
    bt->onError([](const std::string &m) { std::cerr << "Error: " << m << std::endl; });
    if (!bt->connect(connectTarget)) {
        std::cerr << "Failed to connect: " << bt->lastError() << std::endl;
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 1;
    }
    const bool ok = installFirmwareToPairedZowi(qtApp,
                                                *bt,
                                                session,
                                                actionLabel,
                                                a.firmwarePath,
                                                a.batteryTimeout,
                                                a.timeout,
                                                a.forceLowBattery,
                                                a.protocol);
    if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
    return ok ? 0 : 1;
}

int runDisconnect(int argc, char **argv)
{
    zowi::SessionStore store("ZowiDesktop", "ZowiApp");

    std::string savedName = store.getString("activeZowiName");
    std::string savedAddr = store.getString("activeZowiDeviceAddress");

    if (!savedAddr.empty()) {
        std::string label = savedName.empty() ? "(unknown)" : savedName;
        std::cout << "Disconnecting from " << label
                  << " [" << savedAddr << "]..." << std::endl;

        QCoreApplication qtApp(argc, argv);
#ifdef ZOWI_HAVE_NATIVE_BT
        zowi::NativeBluetoothBackend bt;
#else
        zowi::QtBluetoothBackend bt;
#endif

        bool unpairOk = false;
        std::string unpairMsg;
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        bt.onUnpairResult([&](bool ok, const std::string &msg) {
            unpairOk = ok;
            unpairMsg = msg;
            loop.quit();
        });
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(5000);
        bt.unpair(savedAddr);
        loop.exec();

        if (timer.isActive()) timer.stop();
        if (unpairOk) {
            std::cout << "  Bluetooth device removed from system." << std::endl;
        } else {
            std::cout << "  Warning: could not remove Bluetooth device"
                      << (!unpairMsg.empty() ? ": " + unpairMsg : "")
                      << std::endl;
        }

        std::cout << "  Pairing data cleared." << std::endl;
    } else {
        std::cout << "No device was paired." << std::endl;
    }

    store.setString("activeZowiDeviceAddress", "");
    store.setString("activeZowiName", "");
    store.setBool("wizardDismissed", false);
    return 0;
}

int runStatus(int argc, char **argv, const StatusArgs &a)
{
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

    // Resolve the backend: explicit --backend, else the transport used at
    // connect time, else Bluetooth by default.
    std::string backend = a.backend;
    if (backend == "auto") {
        backend = store.getString("activeZowiTransport");
        if (backend == "bt") backend = "bluetooth";
        if (backend.empty()) backend = "bluetooth";
    }

    // Attempt a live connection so the reported state reflects the robot's
    // actual running firmware rather than the (possibly stale) session cache.
    // Use the registered transport (USB or Bluetooth); the address saved for
    // USB is the TTY path, which is what the serial backend expects.
    bool live = false;
    {
        QCoreApplication qtApp(argc, argv);
        resetRobotState();

        std::string target, boundTty;
        auto bt = prepareFlashBackend(backend, addr, a.tty, a.baud, target, boundTty);
        if (!bt) {
            std::cout << "Could not open a backend for '" << addr << "'; showing last known (cached) state." << std::endl;
        } else {
        #ifdef ZOWI_HAVE_SERIAL
    const bool isUsb = (dynamic_cast<SerialBackend *>(bt.get()) != nullptr);
#else
    const bool isUsb = false;
#endif
            const int effTimeout = isUsb ? std::max(a.timeout, 8) : a.timeout;
#ifndef _WIN32
            if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
                std::cout << "Discovering " << target << "..." << std::endl;
                discoverDevice(qtApp, *qtBt, target, kDiscoveryTimeoutMs);
            }
#endif

            bt->onDataReceived([](const std::string &data) { onDataReceived(data); });
            bt->onConnectionChanged([](bool connected) {
                if (connected) std::cout << "Connected. Reading live status..." << std::endl;
            });
            bt->onError([](const std::string &msg) { std::cerr << "Error: " << msg << std::endl; });

            std::cout << "Connecting to " << target << "..." << std::endl;
            bt->connect(target);
            if (waitForRobotIdentity(qtApp, *bt, (effTimeout + 2) * 1000)) {
                live = true;
                if (!g_robotName.empty()) name = g_robotName;
                if (!g_appId.empty()) appId = g_appId;
                if (g_battery >= 0) battery = static_cast<int>(g_battery);

                store.setString("activeZowiName", name);
                store.setString("activeZowiAppId", appId);
                store.setInt("activeZowiBattery", battery);
            }
            bt->disconnect();
            if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        }
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
    return 0;
}

int runControl(int argc, char **argv, const ControlArgs &a)
{
    QCoreApplication qtApp(argc, argv);
    resetRobotState();

    zowi::SessionStore session("ZowiDesktop", "ZowiApp");

    std::string backend = a.backend;
    if (backend == "auto") {
        backend = session.getString("activeZowiTransport");
        if (backend == "bt") backend = "bluetooth";
        if (backend.empty()) backend = "bluetooth";
    }

    const std::string ctrlAddr = a.address.empty()
                                     ? session.getString("activeZowiDeviceAddress")
                                     : a.address;
    if (ctrlAddr.empty()) {
        std::cerr << "No paired device found. Run 'connect' first or pass --address." << std::endl;
        return 1;
    }

    std::string target, boundTty;
    auto bt = prepareFlashBackend(backend, ctrlAddr, a.tty, a.baud, target, boundTty);
    if (!bt) return 1;

#ifdef ZOWI_HAVE_SERIAL
    const bool isUsb = (dynamic_cast<SerialBackend *>(bt.get()) != nullptr);
#else
    const bool isUsb = false;
#endif
    const int effTimeout = isUsb ? std::max(a.timeout, 8) : a.timeout;

    // Speed as an index: 0=slow, 1=medium, 2=fast (matches the original app's
    // speedArray = {2000, 1000, 700}).
    int speedIndex = 1;  // default medium
    if (a.speed == "slow") speedIndex = 0;
    else if (a.speed == "fast") speedIndex = 2;
    else if (a.speed != "medium") {
        std::cerr << "Unknown speed '" << a.speed << "'; using 'medium'.\n";
    }
    auto speedFromIndex = [](int idx) -> zowi::MovementSpeed {
        switch (idx) {
            case 0: return zowi::MovementSpeed::Slow;
            case 2: return zowi::MovementSpeed::Fast;
            default: return zowi::MovementSpeed::Medium;
        }
    };
    auto speedNameFromIndex = [](int idx) -> const char* {
        switch (idx) {
            case 0: return "SLOW";
            case 2: return "FAST";
            default: return "MEDIUM";
        }
    };
    zowi::MovementSpeed speed = speedFromIndex(speedIndex);

#ifndef ZOWI_HAVE_NATIVE_BT
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << target << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, target, kDiscoveryTimeoutMs);
    }
#endif

    std::cout << "Connecting to " << target << "..." << std::endl;

    bt->onDataReceived([](const std::string &data) { onDataReceived(data); });
    bt->onConnectionChanged([&](bool connected) {
        g_connected = connected;
        if (connected) {
            std::cout << "Connected. Drive with the arrow keys (ESC or 'q' to quit)." << std::endl;
        } else {
            std::cout << "\n\nDisconnected." << std::endl;
        }
    });
    bt->onError([&](const std::string &msg) { std::cerr << "Error: " << msg << std::endl; });

    bt->connect(target);

    if (!waitUntil(qtApp, effTimeout * 1000, []() {
            std::lock_guard<std::mutex> lock(g_mtx);
            return g_connected;
        })) {
        std::cerr << "Could not connect to the robot within " << effTimeout << "s." << std::endl;
        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 1;
    }

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
        bt->send(zowi::commandStop());
        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 0;
    }

    std::cout << "Controls:  UP/W = forward   DOWN/S = backward   LEFT/A = moonwalker left   RIGHT/D = moonwalker right   Q = turn left   E = turn right   +/- = speed   (ESC/Ctrl+C=quit)\n";

    using clock = std::chrono::steady_clock;
    auto lastMove = clock::time_point{};  // epoch: no movement yet
    constexpr auto moveDuration = std::chrono::seconds(1);
    std::string lastKeyDisplay;
    std::string lastAction;

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
            if (key == "speed_up" || key == "speed_down") {
                if (key == "speed_up" && speedIndex < 2) speedIndex++;
                if (key == "speed_down" && speedIndex > 0) speedIndex--;
                speed = speedFromIndex(speedIndex);
                lastKeyDisplay = (key == "speed_up") ? "+" : "-";
                lastAction = std::string("speed ") + speedNameFromIndex(speedIndex);
                std::cout << "\x1b[2K\r[SPEED: " << speedNameFromIndex(speedIndex) << "]" << std::flush;
                continue;
            }
            std::string cmd;
            std::string action;
            if (key == "up") { cmd = zowi::commandWalkForward(speed); action = "forward"; }
            else if (key == "down") { cmd = zowi::commandWalkBackward(speed); action = "backward"; }
            else if (key == "left") { cmd = zowi::commandMoonwalkerLeft(speed); action = "moonwalker left"; }
            else if (key == "right") { cmd = zowi::commandMoonwalkerRight(speed); action = "moonwalker right"; }
            else if (key == "turn_left") { cmd = zowi::commandTurnLeft(speed); action = "turn left"; }
            else if (key == "turn_right") { cmd = zowi::commandTurnRight(speed); action = "turn right"; }

            if (!cmd.empty()) {
                bt->send(cmd);
                lastMove = clock::now();
                std::string keyUpper = key;
                std::transform(keyUpper.begin(), keyUpper.end(), keyUpper.begin(), ::toupper);
                lastKeyDisplay = keyUpper;
                lastAction = action;
                std::cout << "\x1b[2K\r[" << keyUpper << "] " << action << std::flush;
            }
        } else if (lastMove.time_since_epoch().count() != 0
                   && clock::now() - lastMove >= moveDuration) {
            bt->send(zowi::commandStop());
            lastMove = {};  // reset so we only send stop once
            std::cout << "\x1b[2K\rStatus: idle. Speed: " << speedNameFromIndex(speedIndex)
                      << ". Last key: " << (lastKeyDisplay.empty() ? "none" : lastKeyDisplay)
                      << " (" << (lastAction.empty() ? "none" : lastAction) << ")" << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    bt->send(zowi::commandStop());
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    disableRawMode();
    bt->disconnect();
    if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
    std::cout << "\n\nStopped. Disconnected. Bye!\n";
    return 0;
}

// Interactive or one-shot servo-trim calibration (protocol commands C/G).
// Interactive mode walks WARNING → LEGS → FEET → CHECK like the Android app,
// sending live G commands so the user sees each change on the robot. Trims are
// clamped to ±60°. Direct mode (all four trims given via --yl/--yr/--rl/--rr)
// persists them with a single C command.
int runCalibrate(int argc, char **argv, const CalibrateArgs &a)
{
    // Shared calibration domain model (clamps, commands, debounce policy).
    zowi::CalibrationSession calibration;

    QCoreApplication qtApp(argc, argv);
    resetRobotState();

    zowi::SessionStore session("ZowiDesktop", "ZowiApp");

    std::string backend = a.backend;
    if (backend == "auto") {
        backend = session.getString("activeZowiTransport");
        if (backend == "bt") backend = "bluetooth";
        if (backend.empty()) backend = "bluetooth";
    }

    const std::string targetAddr = a.address.empty()
                                       ? session.getString("activeZowiDeviceAddress")
                                       : a.address;
    if (targetAddr.empty()) {
        std::cerr << "No paired device found. Run 'connect' first or pass --address." << std::endl;
        return 1;
    }

    std::string target, boundTty;
    auto bt = prepareFlashBackend(backend, targetAddr, a.tty, a.baud, target, boundTty);
    if (!bt) return 1;

#ifdef ZOWI_HAVE_SERIAL
    const bool isUsb = (dynamic_cast<SerialBackend *>(bt.get()) != nullptr);
#else
    const bool isUsb = false;
#endif
    const int effTimeout = isUsb ? std::max(a.timeout, 8) : a.timeout;

#ifndef ZOWI_HAVE_NATIVE_BT
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << target << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, target, kDiscoveryTimeoutMs);
    }
#endif

    std::cout << "Connecting to " << target << "..." << std::endl;

    bt->onDataReceived([](const std::string &data) { onDataReceived(data); });
    bt->onConnectionChanged([&](bool connected) {
        g_connected = connected;
        if (connected) {
            std::cout << "Connected." << std::endl;
        } else {
            std::cout << "\n\nDisconnected." << std::endl;
        }
    });
    bt->onError([&](const std::string &msg) { std::cerr << "Error: " << msg << std::endl; });

    bt->connect(target);

    if (!waitUntil(qtApp, effTimeout * 1000, []() {
            std::lock_guard<std::mutex> lock(g_mtx);
            return g_connected;
        })) {
        std::cerr << "Could not connect to the robot within " << effTimeout << "s." << std::endl;
        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 1;
    }

    const bool lowBattery = waitForBatteryLevel(qtApp, 2000)
                            && g_battery >= 0 && g_battery < kLowBatteryThreshold;

    // Persist trims with a C command and wait for the EEPROM final ack (&&F).
    auto persistTrims = [&](const std::array<int, 4> &t) -> bool {
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_finalAck = false;
        }
        bt->send(zowi::commandSetTrims(t[0], t[1], t[2], t[3]));
        return waitUntil(qtApp, effTimeout * 1000, []() {
            std::lock_guard<std::mutex> lock(g_mtx);
            return g_finalAck;
        });
    };

    // Move the four servos to 90 + trim in real time (volatile G command).
    auto sendServos = [&]() {
        bt->send(calibration.servosCommand());
    };

    // VICTORY gesture (firmware gesture id 12) unless --no-victory.
    auto playVictory = [&]() {
        if (!a.noVictory) {
            bt->send(zowi::commandGesture(12));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    };

    // Reset stored trims and move every servo back to neutral.
    auto resetCalibration = [&]() {
        calibration.reset();
        bt->send(zowi::commandStop());
        std::this_thread::sleep_for(
            std::chrono::milliseconds(zowi::CalibrationSession::kDebounceMs));
    };

    // ── Direct mode: --yl --yr --rl --rr given, no wizard ─────────
    if (a.direct) {
        const std::array<int, 4> in{{a.yl, a.yr, a.rl, a.rr}};
        const std::array<const char *, 4> names{{"YL", "YR", "RL", "RR"}};
        std::array<int, 4> trims{{0, 0, 0, 0}};
        for (size_t i = 0; i < in.size(); ++i) {
            const int clamped = calibration.clamp(in[i]);
            if (clamped != in[i]) {
                std::cout << "Warning: " << names[i] << " trim " << in[i]
                          << " clamped to " << clamped << " (range "
                          << zowi::CalibrationSession::kMinTrim << ".."
                          << zowi::CalibrationSession::kMaxTrim << ").\n";
            }
            trims[i] = clamped;
        }

        std::cout << "Setting trims: YL=" << trims[0] << " YR=" << trims[1]
                  << " RL=" << trims[2] << " RR=" << trims[3] << "\n";

        if (lowBattery) {
            std::cout << "Warning: battery is low (" << g_battery << "%). Consider recharging.\n";
        }

        if (persistTrims(trims)) {
            std::cout << "Trims saved to EEPROM.\n";
        } else {
            std::cerr << "Warning: the robot did not acknowledge the trim save; the trims may not have been persisted.\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        playVictory();

        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 0;
    }

    // ── Interactive wizard ─────────────────────────────────────
    const bool interactive = isatty(g_stdinFd) && enableRawMode();
    if (!interactive) {
        std::cerr << "Calibration requires a terminal. Pass the four trims (--yl --yr --rl --rr) for non-interactive use.\n";
        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 1;
    }
    std::atexit(disableRawMode);
    std::signal(SIGINT, [](int) {
        g_quit.store(true);
        disableRawMode();
    });

    const int kDebounceMs = zowi::CalibrationSession::kDebounceMs;

    using clock = std::chrono::steady_clock;
    int selection = 0;  // index into trims, within the current step's pair
    long long lastServoSendMs = 0;
    bool needSend = false;
    bool finished = false;
    int result = 0;

    // Send G if the debounce allows; otherwise flag a pending flush.
    auto trySendServos = [&](bool force) {
        const long long now = force
                                 ? lastServoSendMs + kDebounceMs + 1
                                 : std::chrono::duration_cast<std::chrono::milliseconds>(
                                       clock::now().time_since_epoch()).count();
        if (calibration.shouldSend(now, lastServoSendMs)) {
            sendServos();
            needSend = false;
        } else {
            needSend = true;
        }
    };

    // Full-screen repaint of the current step.
    auto render = [&]() {
        const int step = calibration.stepIndex();
        std::cout << "\x1b[2J\x1b[H";
        std::cout << "Zowi Calibration — " << target << "\n";
        std::cout << "  YL=" << calibration.trim(0) << "  YR=" << calibration.trim(1)
                  << "  RL=" << calibration.trim(2) << "  RR=" << calibration.trim(3) << "\n";
        std::cout << "  range: " << zowi::CalibrationSession::kMinTrim << ".."
                  << zowi::CalibrationSession::kMaxTrim << " deg\n";
        std::cout << "----------------------------------------------\n";

        switch (step) {
            case 0:
                std::cout << "\nThis will move Zowi's servos to their neutral position\n"
                             "and let you adjust the four trim offsets. Trims are\n"
                             "saved permanently to the robot's EEPROM.\n";
                if (lowBattery) {
                    std::cout << "\nNOTE: battery is low (" << g_battery
                              << "%). Consider recharging.\n";
                }
                std::cout << "\n[y] Continue   [x] Cancel\n";
                break;
            case 1:
                std::cout << "\nStep 1/3 — Legs. Selected servo: "
                          << (selection == 0 ? "YL (left)" : "YR (right)") << "\n\n"
                          << "  <- / a  select left    -> / d  select right\n"
                          << "  up      +10            down    -10\n"
                          << "  +       +1             -       -1\n\n"
                          << "  [n] Next step (feet)   [x] Cancel\n";
                break;
            case 2:
                std::cout << "\nStep 2/3 — Feet. Selected servo: "
                          << (selection == 2 ? "RL (left)" : "RR (right)") << "\n\n"
                          << "  <- / a  select left    -> / d  select right\n"
                          << "  up      +10            down    -10\n"
                          << "  +       +1             -       -1\n\n"
                          << "  [n] Next step (check)  [x] Cancel\n";
                break;
            case 3:
                std::cout << "\nStep 3/3 — Check.\n\n"
                          << "  [t] Test movement (save trims + victory)\n"
                          << "  [r] Restart (reset trims to 0)\n"
                          << "  [c] Confirm and save to EEPROM\n"
                          << "  [x] Cancel\n";
                break;
        }
        std::cout << std::flush;
    };

    auto warningContinue = [&]() {
        resetCalibration();
        calibration.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(kDebounceMs));
        calibration.nextStep();
        selection = 0;
        lastServoSendMs = 0;
        needSend = false;
        render();
    };

    render();

    while (!g_quit.load() && !finished) {
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            if (!g_connected) break;
        }
        qtApp.processEvents();

        if (needSend) trySendServos(false);

        std::string key = readCalibrationKey();
        if (key.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

        bool changed = false;
        const int step = calibration.stepIndex();
        switch (step) {
            case 0:
                if (key == "yes") {
                    warningContinue();
                    changed = true;
                } else if (key == "quit") {
                    finished = true;
                    result = 0;
                    std::cout << "\n\nCalibration cancelled. Trims unchanged.\n";
                }
                break;

            case 1:
            case 2: {
                const int lo = (step == 1) ? 0 : 2;
                const int hi = lo + 1;

                if (key == "select_left") {
                    selection = (selection == lo) ? hi : lo;
                    changed = true;
                } else if (key == "select_right") {
                    selection = (selection == hi) ? lo : hi;
                    changed = true;
                } else {
                    int delta = 0;
                    if (key == "coarse_up") delta = 10;
                    else if (key == "coarse_down") delta = -10;
                    else if (key == "fine_up") delta = 1;
                    else if (key == "fine_down") delta = -1;
                    if (delta != 0) {
                        if (calibration.adjust(selection, delta)) {
                            changed = true;
                            trySendServos(false);  // live move: G 90+trim
                        }
                    } else if (key == "next") {
                        calibration.nextStep();
                        selection = (calibration.stepIndex() == 2) ? 2 : 0;
                        changed = true;
                    } else if (key == "quit") {
                        finished = true;
                        result = 0;
                        std::cout << "\n\nCalibration cancelled. Trims unchanged.\n";
                    }
                }
                break;
            }

            case 3:
                if (key == "test" || key == "confirm") {
                    const std::array<int, 4> trims{{
                        calibration.trim(0), calibration.trim(1),
                        calibration.trim(2), calibration.trim(3)}};
                    if (persistTrims(trims)) {
                        std::cout << "\nTrims saved to EEPROM.\n";
                    } else {
                        std::cerr << "\nWarning: the robot did not acknowledge the trim save; the trims may not have been persisted.\n";
                    }
                    playVictory();
                    if (key == "test") {
                        // Back to the live calibration pose so the user can
                        // keep adjusting.
                        sendServos();
                        render();
                    } else {
                        finished = true;
                        result = 0;
                        std::cout << "\n\nCalibration saved. Bye!\n";
                    }
                } else if (key == "restart") {
                    resetCalibration();
                    calibration.reset();
                    selection = 0;
                    lastServoSendMs = 0;
                    needSend = false;
                    render();
                } else if (key == "quit") {
                    finished = true;
                    result = 0;
                    std::cout << "\n\nCalibration cancelled. Trims unchanged.\n";
                }
                break;
        }

        if (changed && !finished) render();
    }

    disableRawMode();
    bt->disconnect();
    if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
    return result;
}

// ── Helper: list available options for move/gesture/mouth/sing ──────────────
namespace {

void listMovements() {
    std::cout << "Available movements:\n";
    std::cout << "  forward, backward, left, right, moonwalker-left, moonwalker-right\n";
}

void listGestures() {
    std::cout << "Available gestures (1-13):\n";
    std::cout << "  1: happy        2: super-happy   3: sad          4: sleeping\n";
    std::cout << "  5: fart         6: confused      7: love         8: angry\n";
    std::cout << "  9: fretful     10: magic        11: wave        12: victory\n";
    std::cout << " 13: fail\n";
}

void listMouths() {
    std::cout << "Available mouths (0-30):\n";
    std::cout << "  0: zero         1: one           2: two          3: three\n";
    std::cout << "  4: four         5: five          6: six          7: seven\n";
    std::cout << "  8: eight        9: nine         10: smile       11: happy-open\n";
    std::cout << " 12: happy-closed 13: heart       14: big-surprise 15: small-surprise\n";
    std::cout << " 16: tongue-out  17: vamp1        18: vamp2       19: line\n";
    std::cout << " 20: confused     21: diagonal    22: sad         23: sad-open\n";
    std::cout << " 24: sad-closed   25: ok          26: x           27: interrogation\n";
    std::cout << " 28: thunder      29: culito      30: angry\n";
}

void listMelodies() {
    std::cout << "Available melodies (1-19):\n";
    std::cout << "  1: connection    2: disconnection  3: surprise      4: oh-oh\n";
    std::cout << "  5: oh-oh-2       6: cuddly         7: sleeping      8: happy\n";
    std::cout << "  9: super-happy  10: happy-short   11: sad         12: confused\n";
    std::cout << " 13: fart1        14: fart2         15: fart3       16: mode1\n";
    std::cout << " 17: mode2        18: mode3         19: button-pushed\n";
}

// Parse a name-or-id argument into a 0-based enum value.
// Returns -1 on error (unknown name or out-of-range id).
int parseNameOrId(const std::string &arg, const std::vector<std::string> &names, int maxId) {
    // Try numeric first
    try {
        int id = std::stoi(arg);
        if (id >= 0 && id <= maxId) return id;
        return -1;
    } catch (...) {
        // Not numeric, try name
    }
    // Try name (case-insensitive)
    std::string argLower = arg;
    std::transform(argLower.begin(), argLower.end(), argLower.begin(), ::tolower);
    for (size_t i = 0; i < names.size(); ++i) {
        if (names[i] == argLower) return static_cast<int>(i);
    }
    return -1;
}

// Name tables shared by the one-shot subcommands and the shell, so both
// parse names and ids identically (see docs/project/ZOWI_CLI_SHELL.md).
const std::vector<std::string> kGestureNames = {
    "happy", "super-happy", "sad", "sleeping", "fart", "confused",
    "love", "angry", "fretful", "magic", "wave", "victory", "fail"
};

const std::vector<std::string> kMouthNames = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
    "smile", "happy-open", "happy-closed", "heart", "big-surprise", "small-surprise",
    "tongue-out", "vamp1", "vamp2", "line", "confused", "diagonal",
    "sad", "sad-open", "sad-closed", "ok", "x", "interrogation",
    "thunder", "culito", "angry"
};

const std::vector<std::string> kMelodyNames = {
    "connection", "disconnection", "surprise", "oh-oh", "oh-oh-2",
    "cuddly", "sleeping", "happy", "super-happy", "happy-short",
    "sad", "confused", "fart1", "fart2", "fart3",
    "mode1", "mode2", "mode3", "button-pushed"
};

// Builders shared by the one-shot subcommands and the shell. Each returns
// false when the token is unknown; on success cmd holds the protocol string
// and desc a human-readable description for the "Sent: ..." line.

bool buildMoveCommand(const std::string &direction, const std::string &speed,
                      std::string &cmd) {
    zowi::MovementSpeed spd = zowi::MovementSpeed::Medium;
    if (speed == "slow") spd = zowi::MovementSpeed::Slow;
    else if (speed == "fast") spd = zowi::MovementSpeed::Fast;
    else if (speed != "medium") {
        std::cerr << "Unknown speed '" << speed << "'; using 'medium'.\n";
    }

    std::string dirLower = direction;
    std::transform(dirLower.begin(), dirLower.end(), dirLower.begin(), ::tolower);

    if (dirLower == "forward") cmd = zowi::commandWalkForward(spd);
    else if (dirLower == "backward") cmd = zowi::commandWalkBackward(spd);
    else if (dirLower == "left" || dirLower == "turn-left") cmd = zowi::commandTurnLeft(spd);
    else if (dirLower == "right" || dirLower == "turn-right") cmd = zowi::commandTurnRight(spd);
    else if (dirLower == "moonwalker-left") cmd = zowi::commandMoonwalkerLeft(spd);
    else if (dirLower == "moonwalker-right") cmd = zowi::commandMoonwalkerRight(spd);
    else return false;
    return true;
}

bool buildGestureCommand(const std::string &token, std::string &cmd, std::string &desc) {
    // Gesture names are 0-based; the protocol is 1-based.
    int id = parseNameOrId(token, kGestureNames, 12);
    if (id < 0) return false;
    cmd = zowi::commandGesture(id + 1);
    desc = "gesture " + token;
    return true;
}

bool buildMouthCommand(const std::string &token, std::string &cmd, std::string &desc) {
    int id = parseNameOrId(token, kMouthNames, 30);
    if (id < 0) return false;
    cmd = zowi::commandMouthById(static_cast<zowi::MouthId>(id));
    desc = "mouth " + token;
    return true;
}

bool buildSingCommand(const std::string &token, std::string &cmd, std::string &desc) {
    // Melody names are 0-based; the enum value maps to the 1-based protocol id.
    int id = parseNameOrId(token, kMelodyNames, 18);
    if (id < 0) return false;
    cmd = zowi::commandSing(static_cast<zowi::MelodyId>(id));
    desc = "sing " + token;
    return true;
}

// Connect to the robot and send a single command, then disconnect.
// Returns 0 on success, 1 on error.
int sendOneShotCommand(int argc, char **argv,
                       const std::string &address, const std::string &tty,
                       int baud, const std::string &backend, int timeout,
                       const std::string &cmd, const std::string &cmdDesc) {
    QCoreApplication qtApp(argc, argv);
    resetRobotState();

    zowi::SessionStore session("ZowiDesktop", "ZowiApp");

    std::string be = backend;
    if (be == "auto") {
        be = session.getString("activeZowiTransport");
        if (be == "bt") be = "bluetooth";
        if (be.empty()) be = "bluetooth";
    }

    const std::string targetAddr = address.empty()
                                       ? session.getString("activeZowiDeviceAddress")
                                       : address;
    if (targetAddr.empty()) {
        std::cerr << "No paired device found. Run 'connect' first or pass --address." << std::endl;
        return 1;
    }

    std::string target, boundTty;
    auto bt = prepareFlashBackend(be, targetAddr, tty, baud, target, boundTty);
    if (!bt) return 1;

#ifdef ZOWI_HAVE_SERIAL
    const bool isUsb = (dynamic_cast<SerialBackend *>(bt.get()) != nullptr);
#else
    const bool isUsb = false;
#endif
    const int effTimeout = isUsb ? std::max(timeout, 8) : timeout;

#ifndef ZOWI_HAVE_NATIVE_BT
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << target << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, target, kDiscoveryTimeoutMs);
    }
#endif

    std::cout << "Connecting to " << target << "..." << std::endl;

    bt->onDataReceived([](const std::string &data) { onDataReceived(data); });
    bt->onConnectionChanged([&](bool connected) {
        g_connected = connected;
        if (connected) {
            std::cout << "Connected." << std::endl;
        } else {
            std::cout << "Disconnected." << std::endl;
        }
    });
    bt->onError([&](const std::string &msg) { std::cerr << "Error: " << msg << std::endl; });

    bt->connect(target);

    if (!waitUntil(qtApp, effTimeout * 1000, []() {
            std::lock_guard<std::mutex> lock(g_mtx);
            return g_connected;
        })) {
        std::cerr << "Could not connect to the robot within " << effTimeout << "s." << std::endl;
        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 1;
    }

    // The robot resets and plays its connection animation right after the
    // link opens; commands sent during that window are swallowed. Wait until
    // it answers a request before sending the actual command.
    if (!waitForRobotReady(qtApp, *bt, (effTimeout + 2) * 1000)) {
        std::cerr << "Robot not ready (still booting?). Command not sent." << std::endl;
        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 1;
    }

    // Send the command and wait for final ack
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        g_finalAck = false;
    }
    bt->send(cmd);
    std::cout << "Sent: " << cmdDesc << std::endl;

    // Wait briefly for the final ack
    waitUntil(qtApp, 2000, []() {
        std::lock_guard<std::mutex> lock(g_mtx);
        return g_finalAck;
    });

    bt->disconnect();
    if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
    return 0;
}

} // namespace

// ── move subcommand ─────────────────────────────────────────────────────────
int runMove(int argc, char **argv, const MoveArgs &a) {
    if (a.list) {
        listMovements();
        return 0;
    }

    if (a.direction.empty()) {
        std::cerr << "Direction required. Use --list to see available movements." << std::endl;
        return 1;
    }

    // Parse direction (and speed, with the shared unknown-speed warning)
    std::string cmd;
    if (!buildMoveCommand(a.direction, a.speed, cmd)) {
        std::cerr << "Unknown direction '" << a.direction << "'. Use --list to see available movements." << std::endl;
        return 1;
    }

    return sendOneShotCommand(argc, argv, a.address, a.tty, a.baud, a.backend, a.timeout, cmd, "move " + a.direction);
}

// ── gesture subcommand ──────────────────────────────────────────────────────
int runGesture(int argc, char **argv, const GestureArgs &a) {
    if (a.list) {
        listGestures();
        return 0;
    }

    if (a.gesture.empty()) {
        std::cerr << "Gesture required. Use --list to see available gestures." << std::endl;
        return 1;
    }

    std::string cmd, desc;
    if (!buildGestureCommand(a.gesture, cmd, desc)) {
        std::cerr << "Unknown gesture '" << a.gesture << "'. Use --list to see available gestures." << std::endl;
        return 1;
    }

    return sendOneShotCommand(argc, argv, a.address, a.tty, a.baud, a.backend, a.timeout, cmd, desc);
}

// ── mouth subcommand ────────────────────────────────────────────────────────
int runMouth(int argc, char **argv, const MouthArgs &a) {
    if (a.list) {
        listMouths();
        return 0;
    }

    if (a.mouth.empty()) {
        std::cerr << "Mouth required. Use --list to see available mouths." << std::endl;
        return 1;
    }

    std::string cmd, desc;
    if (!buildMouthCommand(a.mouth, cmd, desc)) {
        std::cerr << "Unknown mouth '" << a.mouth << "'. Use --list to see available mouths." << std::endl;
        return 1;
    }

    return sendOneShotCommand(argc, argv, a.address, a.tty, a.baud, a.backend, a.timeout, cmd, desc);
}

// ── sing subcommand ─────────────────────────────────────────────────────────
int runSing(int argc, char **argv, const SingArgs &a) {
    if (a.list) {
        listMelodies();
        return 0;
    }

    if (a.melody.empty()) {
        std::cerr << "Melody required. Use --list to see available melodies." << std::endl;
        return 1;
    }

    std::string cmd, desc;
    if (!buildSingCommand(a.melody, cmd, desc)) {
        std::cerr << "Unknown melody '" << a.melody << "'. Use --list to see available melodies." << std::endl;
        return 1;
    }

    return sendOneShotCommand(argc, argv, a.address, a.tty, a.baud, a.backend, a.timeout, cmd, desc);
}

// ── shell subcommand ────────────────────────────────────────────────────────
// Connect once, then read command lines from stdin and execute each against
// the open connection. Interactive (TTY) or batch (piped) via the same loop.
// See docs/project/ZOWI_CLI_SHELL.md for the design.
namespace {

void printShellStatus() {
    std::lock_guard<std::mutex> lock(g_mtx);
    if (!g_robotName.empty()) std::cout << "  Name:    " << g_robotName << std::endl;
    else std::cout << "  Name:    (not received)" << std::endl;
    if (!g_appId.empty()) std::cout << "  App ID:  " << g_appId << std::endl;
    else std::cout << "  App ID:  (not received)" << std::endl;
    if (g_battery >= 0) std::cout << "  Battery: " << g_battery << "%" << std::endl;
    else std::cout << "  Battery: (not received)" << std::endl;
}

bool shellConnected() {
    std::lock_guard<std::mutex> lock(g_mtx);
    return g_connected;
}

void printShellHelp() {
    std::cout << "Commands:\n"
                 "  move <dir> [speed]   forward, backward, left, right,\n"
                 "                       moonwalker-left, moonwalker-right\n"
                 "                       (speed: slow, medium, fast)\n"
                 "  gesture <name|id>    e.g. gesture happy, gesture 1\n"
                 "  mouth <name|id>      e.g. mouth heart, mouth 13\n"
                 "  sing <name|id>       e.g. sing connection, sing 1\n"
                 "  stop                 stop the current movement\n"
                 "  status               show cached robot identity\n"
                 "  help                 this summary\n"
                 "  quit (exit, q)       disconnect and leave\n"
                 "Lines starting with # are comments.\n";
}

} // namespace

int runShell(int argc, char **argv, const ShellArgs &a)
{
    QCoreApplication qtApp(argc, argv);
    resetRobotState();

    zowi::SessionStore session("ZowiDesktop", "ZowiApp");

    // Pick the backend: explicit --backend, else the transport used at
    // connect time, else Bluetooth by default.
    std::string be = a.backend;
    if (be == "auto") {
        be = session.getString("activeZowiTransport");
        if (be == "bt") be = "bluetooth";
        if (be.empty()) be = "bluetooth";
    }

    const std::string targetAddr = a.address.empty()
                                       ? session.getString("activeZowiDeviceAddress")
                                       : a.address;
    if (targetAddr.empty()) {
        std::cerr << "No paired device found. Run 'connect' first or pass --address." << std::endl;
        return 1;
    }

    std::string target, boundTty;
    auto bt = prepareFlashBackend(be, targetAddr, a.tty, a.baud, target, boundTty);
    if (!bt) return 1;

    // Over USB the robot takes a while to boot and start emitting data, so give
    // it a longer default timeout (8s) unless the user asked for more.
#ifdef ZOWI_HAVE_SERIAL
    const bool isUsb = (dynamic_cast<SerialBackend *>(bt.get()) != nullptr);
#else
    const bool isUsb = false;
#endif
    const int effTimeout = isUsb ? std::max(a.timeout, 8) : a.timeout;

#ifndef ZOWI_HAVE_NATIVE_BT
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << target << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, target, kDiscoveryTimeoutMs);
    }
#endif

    std::cout << "Connecting to " << target << "..." << std::endl;

    bt->onDataReceived([](const std::string &data) { onDataReceived(data); });
    bt->onConnectionChanged([](bool connected) {
        g_connected = connected;
        if (connected) {
            std::cout << "Connected." << std::endl;
        } else {
            std::cout << "\nDisconnected." << std::endl;
        }
    });
    bt->onError([&](const std::string &msg) { std::cerr << "Error: " << msg << std::endl; });

    bt->connect(target);

    if (!waitUntil(qtApp, effTimeout * 1000, []() { return shellConnected(); })) {
        std::cerr << "Could not connect to the robot within " << effTimeout << "s." << std::endl;
        bt->disconnect();
        if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
        return 1;
    }

    // Identify the robot so the session context is visible up front (and so
    // the prompt can show its name). waitForRobotIdentity re-requests every
    // 500 ms until the full identity arrives or its window (floored at 10 s)
    // closes — the first request is often lost to the boot cycle.
    waitForRobotIdentity(qtApp, *bt, (effTimeout + 2) * 1000);
    printShellStatus();
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        if (g_battery >= 0 && g_battery < kLowBatteryThreshold) {
            std::cout << "Warning: battery is low (" << g_battery << "%). Consider recharging.\n";
        }
    }

    const bool interactive = isatty(g_stdinFd) != 0;

    // Prompt shows which robot this session talks to; fall back to the
    // generic name when identification never completed.
    std::string prompt = "ZOWI_CLI:";
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        prompt += g_robotName.empty() ? std::string("Zowi") : g_robotName;
    }
    prompt += "> ";

    if (interactive) {
        std::cout << "Type 'help' for the command list, 'quit' to disconnect.\n";
    }

    while (true) {
        if (interactive) {
            std::cout << prompt << std::flush;
        }
        std::string line;
        if (!std::getline(std::cin, line)) break;  // EOF

        // Trim; skip empty lines and '#' comments.
        const auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        line.erase(0, first);
        if (line[0] == '#') continue;

        if (!shellConnected()) {
            std::cout << "Connection lost." << std::endl;
            break;
        }

        // Split into verb + whitespace-separated arguments.
        std::vector<std::string> tokens;
        {
            std::istringstream iss(line);
            std::string tok;
            while (iss >> tok) tokens.push_back(tok);
        }
        std::string verb = tokens[0];
        std::transform(verb.begin(), verb.end(), verb.begin(), ::tolower);

        if (verb == "quit" || verb == "exit" || verb == "q") break;

        if (verb == "help") {
            printShellHelp();
            continue;
        }

        if (verb == "status") {
            printShellStatus();
            continue;
        }

        std::string cmd, desc;
        if (verb == "move") {
            const std::string dir = tokens.size() > 1 ? tokens[1] : "";
            const std::string speed = tokens.size() > 2 ? tokens[2] : "medium";
            if (dir.empty()) {
                std::cerr << "Usage: move <dir> [speed]. 'help' lists the directions." << std::endl;
                continue;
            }
            if (!buildMoveCommand(dir, speed, cmd)) {
                std::cerr << "Unknown direction '" << dir << "'. 'help' lists the directions." << std::endl;
                continue;
            }
            desc = "move " + dir;
        } else if (verb == "gesture") {
            if (tokens.size() < 2 || !buildGestureCommand(tokens[1], cmd, desc)) {
                std::cerr << "Unknown gesture. Use 'zowi_cli gesture --list' for the names." << std::endl;
                continue;
            }
        } else if (verb == "mouth") {
            if (tokens.size() < 2 || !buildMouthCommand(tokens[1], cmd, desc)) {
                std::cerr << "Unknown mouth. Use 'zowi_cli mouth --list' for the names." << std::endl;
                continue;
            }
        } else if (verb == "sing") {
            if (tokens.size() < 2 || !buildSingCommand(tokens[1], cmd, desc)) {
                std::cerr << "Unknown melody. Use 'zowi_cli sing --list' for the names." << std::endl;
                continue;
            }
        } else if (verb == "stop") {
            cmd = zowi::commandStop();
            desc = "stop";
        } else {
            std::cerr << "Unknown command '" << verb << "'. Type 'help' for the command list." << std::endl;
            continue;
        }

        // Send and wait briefly for the final ack so the robot is not flooded.
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_finalAck = false;
        }
        bt->send(cmd);
        std::cout << "Sent: " << desc << std::endl;
        waitUntil(qtApp, 2000, []() {
            std::lock_guard<std::mutex> lock(g_mtx);
            return g_finalAck;
        });
        qtApp.processEvents();
    }

    if (bt->isConnected()) {
        bt->send(zowi::commandStop());
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    bt->disconnect();
    if (!boundTty.empty()) [[maybe_unused]] int ret = std::system("rfcomm release 0");
    std::cout << "\nDisconnected. Bye!" << std::endl;
    return 0;
}

} // namespace zowi_cli
