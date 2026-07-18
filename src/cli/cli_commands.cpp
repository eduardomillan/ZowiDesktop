#include "cli_commands.h"
#include "cli_state.h"
#include "cli_util.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <csignal>
#include <unistd.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

#include <zowi/session_store.h>
#include <zowi/translation_engine.h>
#include <zowi/config_store.h>
#include <zowi/robot_commands.h>
#include <zowi/protocol.h>
#include <qt_bluetooth_backend.h>
#include <serial_bluetooth_backend.h>

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

int runPorts()
{
    auto ports = zowi::SerialBluetoothBackend::listSerialPorts();
    if (ports.empty()) {
        std::cout << "No USB serial ports found (/dev/ttyUSB*, /dev/ttyACM*)." << std::endl;
    } else {
        std::cout << "Available USB serial ports:" << std::endl;
        for (const auto &p : ports) std::cout << "  " << p << std::endl;
    }
    return 0;
}

int runScan(int argc, char **argv, const ScanArgs &a)
{
    QCoreApplication qtApp(argc, argv);
    zowi::QtBluetoothBackend bt;

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
    zowi::QtBluetoothBackend bt;
    resetRobotState();

    std::cout << "Discovering " << a.address << "..." << std::endl;
    discoverDevice(qtApp, bt, a.address, kDiscoveryTimeoutMs);

    std::cout << "Connecting to " << a.address << "..." << std::endl;

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

    bt.connect(a.address);

    waitForRobotData(qtApp, (a.timeout + 2) * 1000);

    bt.disconnect();

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
    std::cout << "  Address: " << a.address << std::endl;

    zowi::SessionStore store("ZowiDesktop", "ZowiApp");
    store.setString("activeZowiDeviceAddress", a.address);
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
    zowi::QtBluetoothBackend bt;
    resetRobotState();

    zowi::SessionStore session("ZowiDesktop", "ZowiApp");
    std::string savedAddr = session.getString("activeZowiDeviceAddress");
    if (savedAddr.empty()) {
        std::cerr << "No paired device found. Run 'connect' first." << std::endl;
        return 1;
    }

    std::cout << "Discovering " << savedAddr << "..." << std::endl;
    discoverDevice(qtApp, bt, savedAddr, kDiscoveryTimeoutMs);

    std::cout << "Connecting to " << savedAddr << "..." << std::endl;

    std::atomic<bool> renameSent{false};

    bt.onConnectionChanged([&](bool connected) {
        g_connected = connected;
    });

    bt.onDataReceived([&](const std::string &data) {
        onDataReceived(data);
    });

    bt.onError([&](const std::string &msg) {
        std::cerr << "Error: " << msg << std::endl;
    });

    bt.connect(savedAddr);

    bool renamed = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(a.timeout + 2);
    while (std::chrono::steady_clock::now() < deadline) {
        qtApp.processEvents();
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            if (g_connected && g_dataReceived && !renameSent) {
                std::cout << "Connected. Sending rename command..." << std::endl;
                const std::string cmd = zowi::makeCommand(zowi::Command::SetName, a.name);
                bt.send(cmd);
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
    bt.disconnect();

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
    if (auto *qtBt = dynamic_cast<zowi::QtBluetoothBackend *>(bt.get())) {
        std::cout << "Discovering " << connectTarget << "..." << std::endl;
        discoverDevice(qtApp, *qtBt, connectTarget, kDiscoveryTimeoutMs);
    }
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
        if (!boundTty.empty()) std::system("rfcomm release 0");
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
    if (!boundTty.empty()) std::system("rfcomm release 0");
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
        zowi::QtBluetoothBackend bt;

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

int runStatus(int argc, char **argv, int connectTimeout)
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

        std::cout << "Discovering " << addr << "..." << std::endl;
        discoverDevice(qtApp, bt, addr, kDiscoveryTimeoutMs);

        std::cout << "Connecting to " << addr << "..." << std::endl;
        bt.connect(addr);
        if (waitForRobotData(qtApp, (connectTimeout + 2) * 1000)) {
            live = true;
            if (!g_robotName.empty()) name = g_robotName;
            if (!g_appId.empty()) appId = g_appId;
            if (g_battery >= 0) battery = static_cast<int>(g_battery);

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
    return 0;
}

int runControl(int argc, char **argv, const ControlArgs &a)
{
    QCoreApplication qtApp(argc, argv);
    zowi::QtBluetoothBackend bt;
    resetRobotState();

    zowi::MovementSpeed speed = zowi::MovementSpeed::Medium;
    if (a.speed == "slow") speed = zowi::MovementSpeed::Slow;
    else if (a.speed == "fast") speed = zowi::MovementSpeed::Fast;
    else if (a.speed != "medium") {
        std::cerr << "Unknown speed '" << a.speed << "'; using 'medium'.\n";
    }

    zowi::SessionStore session("ZowiDesktop", "ZowiApp");
    const std::string ctrlAddr = a.address.empty()
                                     ? session.getString("activeZowiDeviceAddress")
                                     : a.address;
    if (ctrlAddr.empty()) {
        std::cerr << "No paired device found. Run 'connect' first or pass --address." << std::endl;
        return 1;
    }

    std::cout << "Discovering " << ctrlAddr << "..." << std::endl;
    discoverDevice(qtApp, bt, ctrlAddr, kDiscoveryTimeoutMs);

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

    if (!waitUntil(qtApp, a.timeout * 1000, []() {
            std::lock_guard<std::mutex> lock(g_mtx);
            return g_connected;
        })) {
        std::cerr << "Could not connect to the robot within " << a.timeout << "s." << std::endl;
        bt.disconnect();
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

    bt.send(zowi::commandStop());
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    disableRawMode();
    bt.disconnect();
    std::cout << "\nStopped. Disconnected. Bye!\n";
    return 0;
}

} // namespace zowi_cli
