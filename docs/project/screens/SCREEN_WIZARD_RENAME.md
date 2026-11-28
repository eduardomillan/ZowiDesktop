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
  `setDeviceName(name)`; signals `finalAckReceived` (the `&&F` ack) and `deviceChanged`.
- `Config.get(...)`: `zowi_default_name`, theme colors.
- `Translator` (via `tr()`).

## Commands sent

| Action | Wire command |
|--------|--------------|
| Rename | `R <name>\r` via `Robot.sendData(...)` (persists in the robot's EEPROM) |

## Implementation notes

- `isValidName()` mirrors the core `zowi::isValidRobotName()` (ASCII + Latin-1
  accented letters, excludes `×`/`÷`); invalid chars are stripped while typing.
- **At most 10 bytes** (`zowi::kMaxRobotNameBytes`): the firmware keeps the name in
  an 11-byte buffer with no room for the terminator after the tenth byte (zowiLibs
  `receiveName`). Accented letters take two bytes in UTF-8, so the field counts
  them as 2 (`nameBytes()`), stops accepting characters at 10 and the button stays
  disabled for a longer name. `zowi_cli rename` applies the same rule.
- `dataReady` gate: waits for `deviceName !== "" && appId !== "" &&
  battery >= 0` before sending (mirrors the CLI `waitForRobotData`), because the
  robot plays its connection animation right after the link opens.
- **No fixed wait.** The controls unlock as soon as `dataReady` is true. The old
  `lockTimer` (`rename_lock_ms`, 3 s in `config.json`) is gone: it counted from the
  moment the screen opened, but this screen is only reached after the robot has
  already reported its name, so it just made the user wait. The robot answers the
  identity requests only once its welcome gesture is over, so `dataReady` is the real
  readiness gate.
- **The ack is the parsed signal, not a text search.** The screen completes on
  `Robot.onFinalAckReceived`. It used to look for `"&&F"` inside a raw chunk, but
  Bluetooth sometimes splits the frame (`"&"` then `"&F%%"`): the rename worked in the
  robot, the screen waited 8 s and showed *rename failed*, and the second try "worked"
  only because the robot already had the name (the click handler skips the rename
  then). `ForgetController` had the same flaw and uses the signal too.
- **One automatic retry.** If neither the ack nor the new name has appeared 2.5 s
  after sending (`retryTimer`), the command is sent once more: a command sent while
  the robot is busy or the link stalls can be swallowed (the 28/11 log shows a ~3.4 s
  silent gap after the robot's first reply), and renaming twice to the same name is
  harmless. The user no longer has to press the button again.
- **Lost-ack safety net.** While renaming, if the identity poll reports the requested
  name (`deviceChanged`), the rename is completed as well. Useful over USB, where
  the robot often sends no `&&F`.
- `renameTimer` timeout (5 s over Bluetooth, 8 s in `usbMode`; the ack normally
  arrives within ~100 ms): in `usbMode` the rename is treated as
  best-effort and still fires `renamed(name)`; otherwise it shows
  `rename_failed`.
- Skips the rename when the robot already carries the requested name.