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

## Layout and resizing

A single `ColumnLayout` (no absolutely offset layers), top to bottom: connection
notice (in the flow, so it pushes the content down instead of covering the logo),
flexible space, logo, "ZOWI", "Desktop", Continue / Quit, flexible space, language
label and selector. Nothing overlaps at any window size.

- **Scale:** `unit = min(width, height)`; logo `clamp(unit × 0.30, 96, 380)` (shrinks
  when the window is short), title `clamp(unit × 0.08, 28, 96)`, buttons, fonts and
  margins scale between sane limits. At 1024×600 it matches the previous look
  (≈ 180 px logo).
- Continue and Quit sit side by side, or stacked (`GridLayout`) when the window is
  narrow.
- The logo is loaded at its source size (960×960), so it stays sharp with
  fractional scaling.
- **Wayland (LliureX/KDE):** everything is in logical pixels and the language popup
  is an in-window popup opened **upwards** (never runs off the window). Compositors
  ignore client-side window `x`/`y`, so `main.qml` only uses them as a hint.
  `main.qml` also sets `minimumWidth: 480` / `minimumHeight: 360` (honoured on
  Wayland) and sizes the window from the screen it is on (`Screen.width/height` ×
  `window_size_ratio`, default 0.75) instead of the whole virtual desktop.
  Checked on a virtual KWin Wayland session.
- `screenName: "SplashScreen"` lives on the root item (the window title in
  `main.qml` reads it).
- Enter / Return also continues.

## Implementation notes

- Language `ComboBox` syncs to the current locale on load and persists the
  choice via `Session.saveString("locale", ...)`.
- `Component.onCompleted: splashScope.forceActiveFocus()`.
- A banner warns when no transport is available
  (`!Robot.bluetoothAvailable && !Robot.usbAvailable`).
- Dev reset only accessible when `Config.devMode && Config.devOverlayVisible`
  (overlay with a high `z`).
