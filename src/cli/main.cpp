#include <iostream>
#include <string>

#include <CLI/CLI.hpp>

#include "cli_state.h"
#include "cli_util.h"
#include "cli_commands.h"

int main(int argc, char **argv)
{
    CLI::App app{"Zowi Desktop CLI"};

    // ── session subcommand ────────────────────────────────────
    auto *sessionCmd = app.add_subcommand("session", "Manage session data");
    auto *getSession = sessionCmd->add_subcommand("get", "Get a session value");
    auto *setSession = sessionCmd->add_subcommand("set", "Set a session value");
    auto *listSession = sessionCmd->add_subcommand("list", "List all session keys and values");

    zowi_cli::SessionArgs sessionArgs;
    getSession->add_option("key", sessionArgs.getKey, "Key to read")->required();
    setSession->add_option("key", sessionArgs.setKey, "Key to write")->required();
    setSession->add_option("value", sessionArgs.setValue, "Value to write")->required();
    getSession->final_callback([&]() { sessionArgs.get = true; });
    setSession->final_callback([&]() { sessionArgs.set = true; });
    listSession->final_callback([&]() { sessionArgs.list = true; });

    // ── translate subcommand ──────────────────────────────────
    auto *translateCmd = app.add_subcommand("translate", "Translate a string");
    zowi_cli::TranslateArgs translateArgs;
    translateCmd->add_option("--locale,-l", translateArgs.locale, "Locale (es_ES, ca_ES, en_US)")->default_val("es_ES");
    translateCmd->add_option("--context,-c", translateArgs.context, "Context (e.g. WelcomeScreen.qml)")->default_val("");
    translateCmd->add_option("--source,-s", translateArgs.source, "Source text to translate")->required();

    // ── config subcommand ─────────────────────────────────────
    auto *configCmd = app.add_subcommand("config", "Read config values");
    auto *configGet = configCmd->add_subcommand("get", "Get a config value");
    auto *configList = configCmd->add_subcommand("list", "List all config keys and values");

    zowi_cli::ConfigArgs configArgs;
    configGet->add_option("key", configArgs.key, "Config key to read")->required();
    configGet->final_callback([&]() { configArgs.get = true; });
    configList->final_callback([&]() { configArgs.list = true; });

    // ── ports subcommand ──────────────────────────────────────
    auto *portsCmd = app.add_subcommand("ports", "List available USB serial ports (/dev/ttyUSB*, /dev/ttyACM*)\nUse one of these with 'restore'/'alarm'/'adivinawi' via --backend usb --tty <port>.");

    // ── scan subcommand ───────────────────────────────────────
    auto *scanCmd = app.add_subcommand("scan", "Scan for nearby Zowi robots (5s)\nFilters by name and MAC prefix by default.\nUses BlueZ D-Bus; no root needed.");
    zowi_cli::ScanArgs scanArgs;
    scanCmd->add_option("--timeout,-t", scanArgs.timeout, "Scan duration in seconds")->default_val(5);
    scanCmd->add_flag("--no-filter-name", scanArgs.filterName, "Disable filtering by Zowi name");
    scanCmd->add_flag("--no-filter-mac", scanArgs.filterMac, "Disable filtering by MAC prefix");

    // ── connect subcommand ────────────────────────────────────
    auto *connectCmd = app.add_subcommand("connect", "Connect to a Zowi robot by Bluetooth address (or USB serial)\nReceives robot name, app ID, and battery level.\nUses BlueZ D-Bus by default (no root needed); add --backend usb --tty /dev/ttyUSB0 for USB.");
    zowi_cli::ConnectArgs connectArgs;
    connectCmd->add_option("address", connectArgs.address, "Bluetooth MAC address (e.g. B4:9D:0B:32:41:0E) or USB TTY path (e.g. /dev/ttyUSB0)")->required();
    connectCmd->add_option("--timeout,-t", connectArgs.timeout, "Timeout waiting for robot data (seconds)")->default_val(3);
    connectCmd->add_option("--backend", connectArgs.backend, "Backend: 'auto' (default), 'bluetooth' (BlueZ SPP, no root), 'usb' (USB serial, no Bluetooth)")->default_val("auto");
    connectCmd->add_option("--tty", connectArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    connectCmd->add_option("--baud", connectArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);

    // ── rename subcommand ─────────────────────────────────────
    auto *renameCmd = app.add_subcommand("rename", "Rename the connected Zowi robot\nConnects to the saved device, sends the rename command, and saves the new name.");
    zowi_cli::RenameArgs renameArgs;
    renameCmd->add_option("name", renameArgs.name, "New name for the robot")->required();
    renameCmd->add_option("--timeout,-t", renameArgs.timeout, "Timeout waiting for robot response (seconds)")->default_val(3);
    renameCmd->add_option("--backend", renameArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    renameCmd->add_option("--tty", renameArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    renameCmd->add_option("--baud", renameArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);

    // ── disconnect subcommand ─────────────────────────────────
    auto *disconnectCmd = app.add_subcommand("disconnect", "Disconnect and remove the Zowi robot from the system Bluetooth paired devices\nClears all saved pairing data and removes the device from BlueZ.");

    // ── status subcommand ─────────────────────────────────────
    auto *statusCmd = app.add_subcommand("status", "Show current Zowi connection status\nUses the registered transport (USB or Bluetooth); override with --backend/--tty.");
    zowi_cli::StatusArgs statusArgs;
    statusCmd->add_option("--timeout,-t", statusArgs.timeout, "Timeout waiting for robot data (seconds)")->default_val(3);
    statusCmd->add_option("--backend", statusArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    statusCmd->add_option("--tty", statusArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    statusCmd->add_option("--baud", statusArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);

    // ── restore subcommand ────────────────────────────────────
    auto *restoreCmd = app.add_subcommand("restore", "Restore the original factory firmware to the paired Zowi robot\nUploads the bundled ZOWI_BASE_v2.hex file unless a custom path is provided.");
    zowi_cli::FirmwareArgs restoreArgs;
    restoreArgs.firmwarePath = zowi_cli::kFactoryFirmwarePath;
    restoreArgs.protocol = "stk";
    restoreCmd->add_option("--firmware,-f", restoreArgs.firmwarePath, "Path to the firmware .hex file to upload")->default_val(zowi_cli::kFactoryFirmwarePath);
    restoreCmd->add_option("--timeout,-t", restoreArgs.timeout, "Seconds to wait for firmware restore confirmation")->default_val(10);
    restoreCmd->add_option("--battery-timeout", restoreArgs.batteryTimeout, "Seconds to wait for a battery reading before uploading")->default_val(2);
    restoreCmd->add_flag("--force-low-battery", restoreArgs.forceLowBattery, "Continue even if the reported battery level is below 50%");
    restoreCmd->add_option("--protocol", restoreArgs.protocol, "Upload protocol: 'raw' (stream HEX to custom bootloader) or 'stk' (STK500v1)")->default_val("stk");
    restoreCmd->add_option("--tty", restoreArgs.tty, "Serial TTY to use for flashing (e.g. /dev/rfcomm0 or /dev/ttyUSB0). If omitted, one is bound (serial) or auto-picked (usb).")->default_val("");
    restoreCmd->add_option("--baud", restoreArgs.baud, "Serial baud rate (usb Optiboot is typically 57600 or 115200)")->default_val(9600);
    restoreCmd->add_option("--address,-a", restoreArgs.address, "Robot Bluetooth address (overrides the paired device from the session)")->default_val("");
    restoreCmd->add_option("--backend", restoreArgs.backend, "Backend: 'auto' (default, BlueZ SPP), 'bluetooth' (BlueZ SPP, no root), 'serial' (RFCOMM TTY, needs root/setcap), 'usb' (USB serial, no Bluetooth)")->default_val("auto");

    // ── alarm subcommand ──────────────────────────────────────
    auto *alarmCmd = app.add_subcommand("alarm", "Install the Robot Alarm firmware on the paired Zowi robot\nUploads the bundled ZOWI_Alarm_v2.hex file unless a custom path is provided.");
    zowi_cli::FirmwareArgs alarmArgs;
    alarmArgs.firmwarePath = zowi_cli::kAlarmFirmwarePath;
    alarmArgs.protocol = "stk";
    alarmCmd->add_option("--firmware,-f", alarmArgs.firmwarePath, "Path to the alarm firmware .hex file to upload")->default_val(zowi_cli::kAlarmFirmwarePath);
    alarmCmd->add_option("--timeout,-t", alarmArgs.timeout, "Seconds to wait for alarm firmware confirmation")->default_val(10);
    alarmCmd->add_option("--battery-timeout", alarmArgs.batteryTimeout, "Seconds to wait for a battery reading before uploading")->default_val(2);
    alarmCmd->add_flag("--force-low-battery", alarmArgs.forceLowBattery, "Continue even if the reported battery level is below 50%");
    alarmCmd->add_option("--protocol", alarmArgs.protocol, "Upload protocol: 'raw' (stream HEX to custom bootloader) or 'stk' (STK500v1)")->default_val("stk");
    alarmCmd->add_option("--tty", alarmArgs.tty, "Serial TTY to use for flashing (e.g. /dev/rfcomm0 or /dev/ttyUSB0). If omitted, one is bound (serial) or auto-picked (usb).")->default_val("");
    alarmCmd->add_option("--baud", alarmArgs.baud, "Serial baud rate (usb Optiboot is typically 57600 or 115200)")->default_val(9600);
    alarmCmd->add_option("--address,-a", alarmArgs.address, "Robot Bluetooth address (overrides the paired device from the session)")->default_val("");
    alarmCmd->add_option("--backend", alarmArgs.backend, "Backend: 'auto' (default, BlueZ SPP), 'bluetooth' (BlueZ SPP, no root), 'serial' (RFCOMM TTY, needs root/setcap), 'usb' (USB serial, no Bluetooth)")->default_val("auto");

    // ── adivinawi subcommand ──────────────────────────────────
    auto *adivinawiCmd = app.add_subcommand("adivinawi", "Install the Adivinawi game firmware on the paired Zowi robot\nUploads the bundled ZOWI_Adivinawi_v2.hex file unless a custom path is provided.");
    zowi_cli::FirmwareArgs adivinawiArgs;
    adivinawiArgs.firmwarePath = zowi_cli::kAdivinawiFirmwarePath;
    adivinawiArgs.protocol = "stk";
    adivinawiCmd->add_option("--firmware,-f", adivinawiArgs.firmwarePath, "Path to the adivinawi firmware .hex file to upload")->default_val(zowi_cli::kAdivinawiFirmwarePath);
    adivinawiCmd->add_option("--timeout,-t", adivinawiArgs.timeout, "Seconds to wait for adivinawi firmware confirmation")->default_val(10);
    adivinawiCmd->add_option("--battery-timeout", adivinawiArgs.batteryTimeout, "Seconds to wait for a battery reading before uploading")->default_val(2);
    adivinawiCmd->add_flag("--force-low-battery", adivinawiArgs.forceLowBattery, "Continue even if the reported battery level is below 50%");
    adivinawiCmd->add_option("--protocol", adivinawiArgs.protocol, "Upload protocol: 'raw' (stream HEX to custom bootloader) or 'stk' (STK500v1)")->default_val("stk");
    adivinawiCmd->add_option("--tty", adivinawiArgs.tty, "Serial TTY to use for flashing (e.g. /dev/rfcomm0 or /dev/ttyUSB0). If omitted, one is bound (serial) or auto-picked (usb).")->default_val("");
    adivinawiCmd->add_option("--baud", adivinawiArgs.baud, "Serial baud rate (usb Optiboot is typically 57600 or 115200)")->default_val(9600);
    adivinawiCmd->add_option("--address,-a", adivinawiArgs.address, "Robot Bluetooth address (overrides the paired device from the session)")->default_val("");
    adivinawiCmd->add_option("--backend", adivinawiArgs.backend, "Backend: 'auto' (default, BlueZ SPP), 'bluetooth' (BlueZ SPP, no root), 'serial' (RFCOMM TTY, needs root/setcap), 'usb' (USB serial, no Bluetooth)")->default_val("auto");

    // ── control subcommand ───────────────────────────────────
    auto *controlCmd = app.add_subcommand("control", "Interactive keyboard minigame to drive the Zowi robot\nUse the arrow keys to move; press ESC or 'q' to quit.\nConnects to the paired device (or --address).");
    zowi_cli::ControlArgs controlArgs;
    controlCmd->add_option("--address,-a", controlArgs.address, "Robot Bluetooth address (overrides the paired device from the session)")->default_val("");
    controlCmd->add_option("--speed", controlArgs.speed, "Movement speed: slow, medium, fast")->default_val("medium");
    controlCmd->add_option("--timeout,-t", controlArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(3);

    CLI11_PARSE(app, argc, argv);

    // ── Dispatch ──────────────────────────────────────────────
    if (*sessionCmd)    return zowi_cli::runSession(sessionArgs);
    if (*translateCmd)  return zowi_cli::runTranslate(translateArgs);
    if (*configCmd)     return zowi_cli::runConfig(configArgs);
    if (*portsCmd)      return zowi_cli::runPorts();
    if (*scanCmd)       return zowi_cli::runScan(argc, argv, scanArgs);
    if (*connectCmd)    return zowi_cli::runConnect(argc, argv, connectArgs);
    if (*renameCmd)     return zowi_cli::runRename(argc, argv, renameArgs);
    if (*restoreCmd)    return zowi_cli::runFirmware(argc, argv, restoreArgs, "Factory firmware restore");
    if (*alarmCmd)      return zowi_cli::runFirmware(argc, argv, alarmArgs, "Alarm firmware installation");
    if (*adivinawiCmd)  return zowi_cli::runFirmware(argc, argv, adivinawiArgs, "Adivinawi firmware installation");
    if (*disconnectCmd) return zowi_cli::runDisconnect(argc, argv);
    if (*statusCmd)     return zowi_cli::runStatus(argc, argv, statusArgs);
    if (*controlCmd)    return zowi_cli::runControl(argc, argv, controlArgs);

    return 0;
}
