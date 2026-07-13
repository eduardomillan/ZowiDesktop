#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <CLI/CLI.hpp>
#include <QCoreApplication>
#include <zowi/session_store.h>
#include <zowi/translation_engine.h>
#include <zowi/config_store.h>
#include <qt_bluetooth_backend.h>

static std::mutex g_mtx;
static std::condition_variable g_cv;
static std::string g_robotName;
static std::string g_appId;
static float g_battery = -1.0f;
static bool g_connected = false;
static bool g_dataReceived = false;

static void parseRobotMessage(const std::string &msg)
{
    std::string trimmed = msg;
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\n'), trimmed.end());
    if (trimmed.empty()) return;

    std::lock_guard<std::mutex> lock(g_mtx);

    if (trimmed[0] == 'N' && trimmed.size() > 2 && trimmed[1] == ' ') {
        g_robotName = trimmed.substr(2);
        g_dataReceived = true;
    } else if (trimmed[0] == 'U' && trimmed.size() > 2 && trimmed[1] == ' ') {
        g_appId = trimmed.substr(2);
        g_dataReceived = true;
    } else if (trimmed[0] == 'B' && trimmed.size() > 2 && trimmed[1] == ' ') {
        try { g_battery = std::stof(trimmed.substr(2)); } catch (...) {}
        g_dataReceived = true;
    }
}

static std::string g_dataBuffer;

static void onDataReceived(const std::string &data)
{
    std::lock_guard<std::mutex> lock(g_mtx);
    g_dataBuffer += data;

    // Robot protocol: &&E <name>%%, &&I <appId>%%, &&B <battery>%%
    // Messages arrive in arbitrary chunks; look for "%%" terminators
    while (true) {
        auto pos = g_dataBuffer.find("%%");
        if (pos == std::string::npos) break;

        std::string token = g_dataBuffer.substr(0, pos);
        g_dataBuffer.erase(0, pos + 2);

        // Skip leading whitespace/newlines in token
        std::string::size_type start = token.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        token = token.substr(start);

        // Format: &&<prefix> <value>
        if (token.size() >= 4 && token[0] == '&' && token[1] == '&' && token[3] == ' ') {
            char prefix = token[2];
            std::string value = token.substr(4);
            if (prefix == 'E') {
                g_robotName = value;
                g_dataReceived = true;
            } else if (prefix == 'I') {
                g_appId = value;
                g_dataReceived = true;
            } else if (prefix == 'B') {
                try { g_battery = std::stof(value); } catch (...) {}
                g_dataReceived = true;
            }
        }
    }
}

static bool waitForRobotData(zowi::QtBluetoothBackend &bt, int timeoutMs)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_mtx);
            if (!g_robotName.empty() && !g_appId.empty() && g_battery >= 0)
                return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
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
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(connectTimeout + 2);
        while (std::chrono::steady_clock::now() < deadline) {
            qtApp.processEvents();

            {
                std::lock_guard<std::mutex> lock(g_mtx);
                if (!g_robotName.empty() && !g_appId.empty() && g_battery >= 0)
                    break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

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
                std::string cmd = "R " + newName + "\r\n";
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

        // Wait for connection + robot data (name updates after rename)
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(connectTimeout + 2);
        while (std::chrono::steady_clock::now() < deadline) {
            qtApp.processEvents();
            {
                std::lock_guard<std::mutex> lock(g_mtx);
                if (renameSent && g_dataReceived) break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        bt.disconnect();

        // Save new name
        session.setString("activeZowiName", newName);
        std::cout << "Robot renamed to '" << newName << "'." << std::endl;
        std::cout << "Session updated." << std::endl;
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

        std::cout << "Zowi connected:" << std::endl;
        std::cout << "  Name:    " << (name.empty() ? "(unknown)" : name) << std::endl;
        std::cout << "  Address: " << addr << std::endl;
        std::cout << "  App ID:  " << (appId.empty() ? "(unknown)" : appId) << std::endl;
        std::cout << "  Battery: " << (battery >= 0 ? std::to_string(battery) + "%" : "(unknown)") << std::endl;
        std::cout << "  Wizard:  " << (dismissed ? "completed" : "not completed") << std::endl;
    }

    return 0;
}
