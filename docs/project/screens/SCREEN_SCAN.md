# SCREEN_SCAN — ScanScreen.qml

> Bluetooth device discovery: scans for nearby Zowi devices (filtered by name
> or MAC prefix), lists them, persists the selection and pushes on to the
> found/pair screen.

- **File:** `src/views/screens/ScanScreen.qml`
- **i18n context:** `"ScanScreen.qml"`
- Pushed by `WizardScreen.startClicked` (Bluetooth path).

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `deviceSelected(string name, string address)` | tapping a listed device | `main.qml`: push→ WizardFound |
| `back()` | ScreenTemplate back button (`onBackClicked`) | `main.qml`: `stack.pop()` |

## QML context used

- `Robot`: `scanning`, `startScan()`, `stopScan()`, `connectToDevice(addr)`,
  `disconnectFromDevice()`, `connected`, `deviceAddress`; connections
  `onDeviceDiscovered`, `onScanFinished`, `onConnectionChanged`.
- `Session`: `saveActiveZowiDeviceAddress(address)`,
  `saveActiveZowiName(deviceName)`.
- `Config.get(...)`: `zowi_mac_prefix`, `button_scan_visible`, theme colors.

## Commands sent

- None; connection only (handled by `RobotController`).

## Implementation notes

- Device filter: name contains "zowi" (case-insensitive) OR address starts with
  `zowi_mac_prefix`. Disable with `--no-filter-name`/`--no-filter-mac` in the
  CLI counterpart (`zowi_cli scan`).
- `StackView.onActivated` clears the list and calls `Robot.startScan()`;
  `onDeactivated` stops it.
- Manual "Scan again" button only shown when `button_scan_visible == "true"`.