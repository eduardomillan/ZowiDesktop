# SCREEN_WIZARD_FOUND — WizardFoundScreen.qml

> Shown after a device is selected (or straight away in USB-only mode): pairing
> instructions, the pairing PIN (from config) and a single Pair button. Drives
> the actual connection with `Robot.connectToDevice()` / `Robot.connectUsb()`.

- **File:** `src/views/screens/WizardFoundScreen.qml`
- **i18n context:** `"WizardFoundScreen.qml"`
- Pushed by `ScanScreen.deviceSelected` or by `WizardScreen.startClicked`
  (USB-only path, `usbMode = true`).

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `paired()` | successful connection | `main.qml`: `finishRegistration()` |

## Properties

- `usbMode` (`bool`) — when `true`, the button label is `pair_usb_button` and
  pairing uses `Robot.connectUsb()` instead of `Robot.connectToDevice(addr)`.

## QML context used

- `Robot`: `connected`, `connectToDevice(addr)`, `connectUsb()`,
  `disconnectFromDevice()`; connections `onConnectionChanged`,
  `onErrorOccurred`.
- `Session`: `loadActiveZowiDeviceAddress()` (still paired device during a BT
  coming-back flow).
- `Config.get(...)`: `pairing_code`, `zowi_found_image`, `color_danger`, theme
  colors.

## Commands sent

- None directly; connection and identification are handled by `RobotController`.

## Implementation notes

- `pairingTimer` (10 s) times out the pairing attempt and shows `connect_error`.
- If the robot is already `Robot.connected`, the Pair button emits `paired()`
  immediately.
- `showBackButton` is disabled while a pairing attempt is in flight.