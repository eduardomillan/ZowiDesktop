#include <iostream>
#include <string>

#include <CLI/CLI.hpp>

#include "cli_state.h"
#include "cli_util.h"
#include "cli_commands.h"

// Default for the robot-facing --timeout options (bounds the Bluetooth SPP
// connect phase and the robot-data wait). The HC-05 on the robot can take
// longer than 3 s to establish SPP, so the floor is 5 s; per-command -t
// overrides it. Not used by `scan` (scan duration) or the firmware commands
// (10 s confirmation waits).
constexpr int kDefaultRobotTimeout = 5;

int main(int argc, char **argv)
{
    CLI::App app{"Zowi Desktop CLI"};
    app.set_version_flag("--version", std::string(ZOWI_VERSION));

    // ── session subcommand ────────────────────────────────────
    auto *sessionCmd = app.add_subcommand("session", "Manage session data");
    auto *getSession = sessionCmd->add_subcommand("get", "Get a session value");
    auto *setSession = sessionCmd->add_subcommand("set", "Set a session value");
    auto *clearSession = sessionCmd->add_subcommand("clear", "Clear all session data");
    auto *listSession = sessionCmd->add_subcommand("list", "List all session keys and values");

    zowi_cli::SessionArgs sessionArgs;
    getSession->add_option("key", sessionArgs.getKey, "Key to read")->required();
    setSession->add_option("key", sessionArgs.setKey, "Key to write")->required();
    setSession->add_option("value", sessionArgs.setValue, "Value to write")->required();
    getSession->final_callback([&]() { sessionArgs.get = true; });
    setSession->final_callback([&]() { sessionArgs.set = true; });
    clearSession->final_callback([&]() { sessionArgs.clear = true; });
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
    connectCmd->add_option("--timeout,-t", connectArgs.timeout, "Timeout waiting for robot data (seconds)")->default_val(kDefaultRobotTimeout);
    connectCmd->add_option("--backend", connectArgs.backend, "Backend: 'auto' (default), 'bluetooth' (BlueZ SPP, no root), 'usb' (USB serial, no Bluetooth)")->default_val("auto");
    connectCmd->add_option("--tty", connectArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    connectCmd->add_option("--baud", connectArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);

    // ── rename subcommand ─────────────────────────────────────
    auto *renameCmd = app.add_subcommand("rename", "Rename the connected Zowi robot\nConnects to the saved device, sends the rename command, and saves the new name.");
    zowi_cli::RenameArgs renameArgs;
    renameCmd->add_option("name", renameArgs.name, "New name for the robot")->required();
    renameCmd->add_option("--timeout,-t", renameArgs.timeout, "Timeout waiting for robot response (seconds)")->default_val(kDefaultRobotTimeout);
    renameCmd->add_option("--backend", renameArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    renameCmd->add_option("--tty", renameArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    renameCmd->add_option("--baud", renameArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);

    // ── disconnect subcommand ─────────────────────────────────
    auto *disconnectCmd = app.add_subcommand("disconnect", "Disconnect and remove the Zowi robot from the system Bluetooth paired devices\nClears all saved pairing data and removes the device from BlueZ.");

    // ── status subcommand ─────────────────────────────────────
    auto *statusCmd = app.add_subcommand("status", "Show current Zowi connection status\nUses the registered transport (USB or Bluetooth); override with --backend/--tty.");
    zowi_cli::StatusArgs statusArgs;
    statusCmd->add_option("--timeout,-t", statusArgs.timeout, "Timeout waiting for robot data (seconds)")->default_val(kDefaultRobotTimeout);
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
    auto *controlCmd = app.add_subcommand("control", "Interactive keyboard minigame to drive the Zowi robot\nUse the arrow keys or WASD to move; Q/E to turn; press ESC or Ctrl+C to quit.\nConnects to the paired device (or --address). Add --backend usb --tty /dev/ttyUSB0 for USB.");
    zowi_cli::ControlArgs controlArgs;
    controlCmd->add_option("--address,-a", controlArgs.address, "Robot Bluetooth address (overrides the paired device from the session) or USB TTY path (e.g. /dev/ttyUSB0)")->default_val("");
    controlCmd->add_option("--speed", controlArgs.speed, "Movement speed: slow, medium, fast")->default_val("medium");
    controlCmd->add_option("--timeout,-t", controlArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(kDefaultRobotTimeout);
    controlCmd->add_option("--backend", controlArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    controlCmd->add_option("--tty", controlArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    controlCmd->add_option("--baud", controlArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);

    // ── calibrate subcommand ────────────────────────────────
    auto *calibCmd = app.add_subcommand("calibrate", "Calibrate Zowi servo trims (C/G protocol)\nInteractive wizard by default; pass all four trims to apply them non-interactively.\nTrims are clamped to +/-60 degrees.");
    zowi_cli::CalibrateArgs calibArgs;
    calibCmd->add_option("--address,-a", calibArgs.address, "Robot Bluetooth address (overrides the paired device from the session) or USB TTY path (e.g. /dev/ttyUSB0)")->default_val("");
    calibCmd->add_option("--timeout,-t", calibArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(kDefaultRobotTimeout);
    calibCmd->add_option("--backend", calibArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    calibCmd->add_option("--tty", calibArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    calibCmd->add_option("--baud", calibArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);
    calibCmd->add_flag("--no-victory,-N", calibArgs.noVictory, "Skip the victory animation when calibration is confirmed");
    calibCmd->add_option("--yl", calibArgs.yl, "Left leg trim (all four trims are required for direct mode)");
    calibCmd->add_option("--yr", calibArgs.yr, "Right leg trim (all four trims are required for direct mode)");
    calibCmd->add_option("--rl", calibArgs.rl, "Left foot trim (all four trims are required for direct mode)");
    calibCmd->add_option("--rr", calibArgs.rr, "Right foot trim (all four trims are required for direct mode)");

    // ── move subcommand ────────────────────────────────────────
    auto *moveCmd = app.add_subcommand("move", "Move the robot for a number of gait cycles, then stop\nUse --list to see directions, speeds and syntax.");
    zowi_cli::MoveArgs moveArgs;
    moveCmd->add_option("direction", moveArgs.direction, "Movement direction: forward/fw, backward/bk, left/lf, right/rg, moonwalker-left/ml, moonwalker-right/mr (case-insensitive)");
    moveCmd->add_option("cycles", moveArgs.cycles, "Gait cycles to run (>= 1, default 1). The robot stops automatically afterwards")->default_val(1);
    moveCmd->add_option("--speed,-s", moveArgs.speed, "Movement speed: slow/s, medium/m (default), fast/f")->default_val("medium");
    moveCmd->add_option("--address,-a", moveArgs.address, "Robot Bluetooth address (overrides the paired device from the session) or USB TTY path")->default_val("");
    moveCmd->add_option("--timeout,-t", moveArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(kDefaultRobotTimeout);
    moveCmd->add_option("--backend", moveArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    moveCmd->add_option("--tty", moveArgs.tty, "Serial TTY to use for USB")->default_val("");
    moveCmd->add_option("--baud", moveArgs.baud, "Serial baud rate")->default_val(115200);
    moveCmd->add_flag("--list,-l", moveArgs.list, "List available movements and exit");

    // ── gesture subcommand ─────────────────────────────────────
    auto *gestureCmd = app.add_subcommand("gesture", "Send a gesture command to the robot\nUse --list to see available gestures.");
    zowi_cli::GestureArgs gestureArgs;
    gestureCmd->add_option("gesture", gestureArgs.gesture, "Gesture name or ID (1-13)");
    gestureCmd->add_option("--address,-a", gestureArgs.address, "Robot Bluetooth address (overrides the paired device from the session) or USB TTY path")->default_val("");
    gestureCmd->add_option("--timeout,-t", gestureArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(kDefaultRobotTimeout);
    gestureCmd->add_option("--backend", gestureArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    gestureCmd->add_option("--tty", gestureArgs.tty, "Serial TTY to use for USB")->default_val("");
    gestureCmd->add_option("--baud", gestureArgs.baud, "Serial baud rate")->default_val(115200);
    gestureCmd->add_flag("--list,-l", gestureArgs.list, "List available gestures and exit");

    // ── mouth subcommand ───────────────────────────────────────
    auto *mouthCmd = app.add_subcommand("mouth", "Send a mouth/LED pattern to the robot\nUse --list to see available mouths.");
    zowi_cli::MouthArgs mouthArgs;
    mouthCmd->add_option("mouth", mouthArgs.mouth, "Mouth name, ID (0-30), or raw 0/1 binary pattern (value semantics: missing digits are leading zeros, max 32 digits)");
    mouthCmd->add_option("--address,-a", mouthArgs.address, "Robot Bluetooth address (overrides the paired device from the session) or USB TTY path")->default_val("");
    mouthCmd->add_option("--timeout,-t", mouthArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(kDefaultRobotTimeout);
    mouthCmd->add_option("--backend", mouthArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    mouthCmd->add_option("--tty", mouthArgs.tty, "Serial TTY to use for USB")->default_val("");
    mouthCmd->add_option("--baud", mouthArgs.baud, "Serial baud rate")->default_val(115200);
    mouthCmd->add_flag("--list,-l", mouthArgs.list, "List available mouths and exit");

    // ── sing subcommand ────────────────────────────────────────
    auto *singCmd = app.add_subcommand("sing", "Send a melody/sing command to the robot\nUse --list to see available melodies.");
    zowi_cli::SingArgs singArgs;
    singCmd->add_option("melody", singArgs.melody, "Melody name or ID (1-19)");
    singCmd->add_option("--address,-a", singArgs.address, "Robot Bluetooth address (overrides the paired device from the session) or USB TTY path")->default_val("");
    singCmd->add_option("--timeout,-t", singArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(kDefaultRobotTimeout);
    singCmd->add_option("--backend", singArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    singCmd->add_option("--tty", singArgs.tty, "Serial TTY to use for USB")->default_val("");
    singCmd->add_option("--baud", singArgs.baud, "Serial baud rate")->default_val(115200);
    singCmd->add_flag("--list,-l", singArgs.list, "List available melodies and exit");

    // ── shell subcommand ──────────────────────────────────────
    auto *shellCmd = app.add_subcommand("shell", "Persistent-connection command shell\nConnects once, then reads commands from stdin (REPL when interactive, batch when piped):\nmove <dir> [cycles] [speed] | gesture <name|id> | mouth <name|id|0/1> | sing <name|id> | stop | status | help | quit.");
    zowi_cli::ShellArgs shellArgs;
    shellCmd->add_option("--address,-a", shellArgs.address, "Robot Bluetooth address (overrides the paired device from the session) or USB TTY path")->default_val("");
    shellCmd->add_option("--timeout,-t", shellArgs.timeout, "Timeout waiting for connection (seconds)")->default_val(kDefaultRobotTimeout);
    shellCmd->add_option("--backend", shellArgs.backend, "Backend: 'auto' (uses the registered transport), 'bluetooth', or 'usb'")->default_val("auto");
    shellCmd->add_option("--tty", shellArgs.tty, "Serial TTY to use for USB (e.g. /dev/ttyUSB0)")->default_val("");
    shellCmd->add_option("--baud", shellArgs.baud, "Serial baud rate (control firmware uses 115200; 57600 is only for the USB bootloader/flashing)")->default_val(115200);

    CLI11_PARSE(app, argc, argv);

    // Direct mode only when at least one trim option was explicitly given.
    calibArgs.direct = calibCmd->count("--yl") || calibCmd->count("--yr")
                       || calibCmd->count("--rl") || calibCmd->count("--rr");

    // ── Logging setup ────────────────────────────────────────────
    zowi_cli::loadLogLevel();
    zowi_cli::installLogHandler();

    // ── Dispatch ──────────────────────────────────────────────────
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
    if (*calibCmd)      return zowi_cli::runCalibrate(argc, argv, calibArgs);
    if (*moveCmd)       return zowi_cli::runMove(argc, argv, moveArgs);
    if (*gestureCmd)    return zowi_cli::runGesture(argc, argv, gestureArgs);
    if (*mouthCmd)      return zowi_cli::runMouth(argc, argv, mouthArgs);
    if (*singCmd)       return zowi_cli::runSing(argc, argv, singArgs);
    if (*shellCmd)      return zowi_cli::runShell(argc, argv, shellArgs);

    std::cout << app.help() << std::endl;
    return 0;
}
