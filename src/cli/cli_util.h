#ifndef ZOWI_CLI_UTIL_H
#define ZOWI_CLI_UTIL_H

#include <string>
#include <functional>
#include <memory>

#include <QCoreApplication>
#include <zowi/bluetooth_api.h>
#include <zowi/session_store.h>
#include <qt_bluetooth_backend.h>

namespace zowi_cli {

// Pumps the Qt event loop until predicate() is true or timeoutMs elapses.
bool waitUntil(QCoreApplication &qtApp, int timeoutMs,
               const std::function<bool()> &predicate);

// Scan until the given Bluetooth address is discovered by BlueZ, so that a
// subsequent connectToService() does not fail with "Unknown remote device".
bool discoverDevice(QCoreApplication &qtApp, zowi::QtBluetoothBackend &bt,
                    const std::string &address, int timeoutMs);

// ── Interactive keyboard input (raw terminal mode) ───────────
// The minigame reads keys directly from stdin. We switch the terminal to
// raw mode (no line buffering, no echo) and decode the ANSI escape sequences
// the cursor keys produce:
//   ↑ = ESC [ A   ↓ = ESC [ B   → = ESC [ C   ← = ESC [ D
// (Some terminals emit ESC O A/B/C/D instead of ESC [ A/B/C/D.)
//
// Key mappings (like the GUI M3 control pad):
//   Movement keys                   → movement
//   Up / W / w                      → walk forward
//   Down / S / s                    → walk backward
//   Left / A / a                    → moonwalker left
//   Right / D / d                   → moonwalker right
//   Q / q                           → turn left
//   E / e                           → turn right
//   +                               → increase speed (slow→medium→fast)
//   -                               → decrease speed (fast→medium→slow)
//   Ctrl-C (0x03) / ESC             → quit
bool enableRawMode();
void disableRawMode();

// Returns a logical key token ("up"/"down"/"left"/"right"/"turn_left"/"turn_right"
// "speed_up"/"speed_down"/"quit"), or "" if no (complete) key is available.
std::string readKey();

// Waits until name, app id and battery have all been received.
bool waitForRobotData(QCoreApplication &qtApp, int timeoutMs);

// Asks the robot for its name, app id and battery level (E / I / B commands).
// The robot only reports these on request, so call this right after connecting.
void requestRobotData(zowi::BluetoothApi &bt);

// Waits until a battery level has been received.
bool waitForBatteryLevel(QCoreApplication &qtApp, int timeoutMs);

// Waits until the robot reports an app id that differs from previousAppId
// (it reboots into the new firmware after flashing).
bool waitForAppId(QCoreApplication &qtApp, zowi::BluetoothApi &bt, int timeoutMs,
                  const std::string &previousAppId);

// ── Firmware upload (delegates to the zowi_firmware library) ──
// protocol: "raw" streams the Intel HEX file to the device's custom bootloader;
//           "stk" uses the STK500v1 / Optiboot framing.
bool uploadFirmware(zowi::BluetoothApi &bt, QCoreApplication &qtApp,
                    const std::string &firmwarePath, const std::string &protocol);

// Prepare a backend for firmware flashing. Returns either a Qt Bluetooth SPP
// backend (default, no root needed) or a serial TTY backend.
// The serial backend is used when:
//   - --backend serial (RFCOMM over Bluetooth; binds /dev/rfcomm0 if needed), or
//   - --backend usb / --tty is given (a plain USB serial link, no Bluetooth).
// On serial+Bluetooth, binds an RFCOMM TTY if ttyOpt is empty (needs
// root/CAP_NET_ADMIN); the bound path is set in boundTty so the caller can
// release it afterwards. On USB no binding is performed.
// connectTarget is set to the string that should be passed to bt->connect()
// — either the TTY path (serial) or the Bluetooth address (Qt).
// baud selects the serial line speed (9600 for the RFCOMM/ZUM bootloader,
// typically 57600/115200 for USB Optiboot).
std::unique_ptr<zowi::BluetoothApi> prepareFlashBackend(
    const std::string &backendName, const std::string &address,
    const std::string &ttyOpt, int baud, std::string &connectTarget,
    std::string &boundTty);

// Establish a stable connection and upload firmware to the paired robot,
// updating the session store on success. Returns true on success.
bool installFirmwareToPairedZowi(QCoreApplication &qtApp,
                                 zowi::BluetoothApi &bt,
                                 zowi::SessionStore &session,
                                 const std::string &actionLabel,
                                 const std::string &firmwarePath,
                                 int batteryTimeoutSeconds,
                                 int confirmationTimeoutSeconds,
                                 bool forceLowBattery,
                                 const std::string &protocol);

} // namespace zowi_cli

#endif // ZOWI_CLI_UTIL_H
