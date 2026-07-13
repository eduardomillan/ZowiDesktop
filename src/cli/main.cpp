#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <CLI/CLI.hpp>
#include <QCoreApplication>
#include <zowi/session_store.h>
#include <zowi/translation_engine.h>
#include <zowi/config_store.h>
#include <qt_bluetooth_backend.h>

int main(int argc, char **argv)
{
    CLI::App app{"Zowi Desktop CLI"};

    // ── session subcommand ────────────────────────────────────
    std::string sessionFile;
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
    auto *scanCmd = app.add_subcommand("scan", "Scan for nearby Zowi robots (5s)\nRun with sudo to avoid CAP_NET_ADMIN warning.");
    int scanTimeout = 5;
    scanCmd->add_option("--timeout,-t", scanTimeout, "Scan duration in seconds")->default_val(5);

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
            // Detect type: try bool first, then int, then string
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

        // Try to load from filesystem
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

        std::atomic<int> deviceCount{0};

        bt.onDeviceFound([&](const zowi::DeviceInfo &dev) {
            deviceCount++;
            std::cout << dev.name << " [" << dev.address << "]" << std::endl;
        });

        bt.onError([&](const std::string &msg) {
            std::cerr << "Error: " << msg << std::endl;
        });

        std::cout << "Scanning for " << scanTimeout << "s..." << std::endl;
        bt.startDiscovery();

        // Process events for scanTimeout seconds
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

    return 0;
}
