# SCREEN_SETTINGS — SettingsScreen.qml

> App configuration hub. Shows the connection status and **situation-based
> contextual actions** (retry / forget / register / refresh / USB quick-connect)
> plus an option list: restore firmware, rename Zowi, search updates (stub),
> **change language**, delete achievements (stub), forget Zowi, calibrate and
> Hospital (external link).

- **File:** `src/views/screens/SettingsScreen.qml`
- **i18n context:** `"SettingsScreen.qml"`
- Pushed by `HomeScreen.settingsClicked`.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `renameRequested()` | "Rename" option | `main.qml`: push→ WizardRename |
| `forgetCompleted()` | "Forget" flow finished | `main.qml`: replace→ Welcome |
| `calibrationRequested()` | "Calibrate" option | `main.qml`: push→ Calibration |
| `languageRequested()` | "Language" option | `main.qml`: replace the **whole stack** (`stack.replace(null, …)`) with Splash, where the language is picked; "Continue" then goes to Home again (registered Zowi, wizard flag and connection untouched) |

## QML context used

- `Robot`: `situation` (SituationConnected/Connecting/Disconnected/
  TransportLost/Unregistered/Demo), `activeTransport` (TransportUsb/
  TransportBluetooth), `connected`, `usbAvailable`, `appId`,
  `connectUsb()`, `connectToDevice(addr)`, `refreshTransports()`,
  `setTransportPreference(Robot.TransportAuto)`,
  `restoreFirmware(Config.get("factory_firmware_path"))`,
  `confirmRestoreBattery(bool)`; connections `onFirmwareRestoreStarted`,
  `onFirmwareRestoreProgress`, `onFirmwareRestoreFinished`,
  `onFirmwareRestoreBatteryLow`, `onUsbIdentityMismatch`, `onSituationChanged`,
  `onConnectingChanged`.
- `Session`: `loadActiveZowiTransport()`, `loadActiveZowiName()`,
  `loadActiveZowiDeviceAddress()`, `saveWizardDismissed(false)`; connection
  `onSessionChanged`.
- `Config.get(...)`: `factory_firmware_path`, `hospital_url`,
  `message_duration`, theme colors.
- `ForgetController` + `MessageBar` (notifications).

## Commands sent

- None as raw strings. Firmware restore is driven by
  `Robot.restoreFirmware(path)` (STK500v1 flash of `factory_firmware_path`).
  Base firmware is detected via the app ID (`Robot.appId === "ZOWI_BASE_v2"`).

## Implementation notes

- **Situation model drives the actions**: Disconnected/TransportLost → retry +
  forget; Unregistered → register; Demo → refresh transports; plus a USB
  quick-connect whenever USB is present and not connected.
- Options are gated: `connGated` (restore/rename/calibrate need `Robot.connected`),
  `needsRegistration` (forget), and `enabledWhen` predicates (restore disabled on
  base firmware; calibrate enabled only on base firmware).
- Restore shows a live progress bar and a low-battery confirmation dialog
  (`onFirmwareRestoreBatteryLow` → `confirmRestoreBattery`). The dialog text quotes
  the real threshold (`Robot.lowBatteryThreshold`, config key
  `low_battery_threshold`, default 50 %).
- The "Language" row is text-only like the others, always enabled (`connGated: false`).
- **Delete achievements** (key `achievements`) is hidden when the layer is off
  (`achievements_enabled`). Otherwise it opens a confirmation dialog
  (`delete_achievements_*` keys) and on "Delete" calls `Achievements.resetAll()`:
  achievements, usage counters and first visits go back to the initial state; rankings
  and the rest of the data are untouched.