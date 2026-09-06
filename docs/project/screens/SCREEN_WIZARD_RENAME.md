# SCREEN_WIZARD_RENAME — WizardRenameScreen.qml

> Appears right after successful pairing (or from Settings → "Rename Zowi"):
> lets the user give Zowi a friendly name, sends the rename command and waits
> for the firmware `&&F` ack before leaving. Shared by the wizard and Settings.

- **File:** `src/views/screens/WizardRenameScreen.qml`
- **i18n context:** `"WizardRenameScreen.qml"`
- Pushed by `main.qml` `_routeAfterNameCheck()` (wizard) or by
  `SettingsScreen.renameRequested`.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `renamed(string name)` | rename ack received (or USB timeout fallback) | `main.qml` / Settings: `Session.saveActiveZowiName(name)` (+ pop/replace) |

## Properties

- `usbMode` (`bool`) — set by the caller for the USB-only wizard flow.

## QML context used

- `Robot`: `connected`, `deviceName`, `appId`, `battery`, `sendData(...)`,
  `setDeviceName(name)`; connection `onDataReceived` (detects `&&F`).
- `Config.get(...)`: `zowi_default_name`, `rename_lock_ms`, theme colors.
- `Translator` (via `tr()`).

## Commands sent

| Action | Wire command |
|--------|--------------|
| Rename | `R <name>\r` via `Robot.sendData(...)` (persists in the robot's EEPROM) |

## Implementation notes

- `isValidName()` mirrors the core `zowi::isValidRobotName()` (ASCII + Latin-1
  accented letters, excludes `×`/`÷`); invalid chars are stripped while typing.
- `dataReady` gate: waits for `deviceName !== "" && appId !== "" &&
  battery >= 0` before sending (mirrors the CLI `waitForRobotData`), because the
  robot plays its connection animation right after the link opens.
- `lockTimer` (`rename_lock_ms`, default 1500 ms) covers the
  post-connect welcome-gesture busy link.
- `renameTimer` (8 s) timeout: in `usbMode` the rename is treated as
  best-effort and still fires `renamed(name)`; otherwise it shows
  `rename_failed`.
- Skips the rename when the robot already carries the requested name.