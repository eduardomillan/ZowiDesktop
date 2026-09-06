# SCREEN_HOME — HomeScreen.qml

> Main dashboard: two swipeable pages — "Zowi Apps" (games/modes) and
> "Projects" (all disabled placeholders). Top bar with Settings and
> Achievements buttons. Auto-reconnects to the robot on launch.

- **File:** `src/views/screens/HomeScreen.qml`
- **i18n context:** `"HomeScreen.qml"`
- Reached from Splash (`replace`) and after registration/wizard dismissal.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `settingsClicked()` | Settings button | `main.qml`: push→ Settings |
| `achievementsClicked()` | Achievements button | currently a stub (`console.log`) |
| `gamepadClicked()` | Gamepad app card | `main.qml`: push→ Pad |
| `mouthEditorClicked()` | Mouths editor app card | `main.qml`: push→ MouthEditor |
| `goSplash()` / `goWelcome()` | DEV nav buttons | `main.qml`: replace→ Splash/Welcome |

## QML context used

- `Robot`: `connected`, `appId`, `battery`, `situation`,
  `SituationTransportLost`, `activeTransport`, `TransportUsb`,
  `usbAvailable`, `connectToDevice(addr)`, `connectUsb()`.
- `Session`: `loadActiveZowiDeviceAddress()`.
- `Config.get(...)`: theme colors; `Config.devMode`, `Config.devOverlayVisible`
  (DEV nav row).
- `Translator` (via `tr()`).

## Commands sent

- None directly. Auto-connect navigates the backend via `Robot.connectUsb()` or
  `Robot.connectToDevice(savedAddress)`.

## Implementation notes

- Auto-connect on `Component.onCompleted`: USB wins when it is the registered
  transport and available; otherwise reconnect to the saved Bluetooth address.
- The apps model only enables `gamepad` and `mouths_editor`; the rest
  (timeline, zowi_says, mouths) are placeholders.
- App buttons are gated by `home.robotReady`
  (`Robot.connected && appId !== "" && battery >= 0`); disabled cards are
  grayed out.
- DEV navigation row (Splash / Welcome) visible only with
  `Config.devMode && Config.devOverlayVisible`.