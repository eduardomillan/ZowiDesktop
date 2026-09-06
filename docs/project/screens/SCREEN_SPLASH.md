# SCREEN_SPLASH — SplashScreen.qml

> Initial launch screen: Zowi logo/title, Continue / Quit buttons, a language
> selector (ES/CA/EN/FR/BG), a "no connection available" banner when neither
> Bluetooth nor USB is present, and a dev-only "reset" (forget Zowi) button.

- **File:** `src/views/screens/SplashScreen.qml`
- **i18n context:** `"SplashScreen.qml"`
- **Initial item** of the `StackView` in `main.qml`.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `splashFinished()` | Continue button | `main.qml`: replace→ Home (wizard dismissed or device registered) or Welcome |
| `quitRequested()` | Quit button | `main.qml`: `Qt.quit()` |

## QML context used

- `Robot`: `bluetoothAvailable`, `usbAvailable`, `deviceAddress`.
- `Session`: `loadActiveZowiDeviceAddress()`, `saveString("locale", loc)`.
- `Translator`: `currentLocale()`, `load(loc)`.
- `Config.get(...)`: `splash_image`, `button_quit_visible` and theme colors;
  `Config.devMode`, `Config.devOverlayVisible` (dev reset button).
- `ForgetController` (`ForgetController.qml`) + `MessageBar` for the dev reset.

## Commands sent

- None directly. The dev reset uses `ForgetController`, which may send the
  factory rename (`R <zowi_default_name>`) if the robot is reachable.

## Implementation notes

- Language `ComboBox` syncs to the current locale on load and persists the
  choice via `Session.saveString("locale", ...)`.
- `Component.onCompleted: splash.forceActiveFocus()`.
- A `banner` rectangle warns when no transport is available
  (`!Robot.bluetoothAvailable && !Robot.usbAvailable`).
- Dev reset only accessible when `Config.devMode && Config.devOverlayVisible`.