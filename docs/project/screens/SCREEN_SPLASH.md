# SCREEN_SPLASH — SplashScreen.qml

> Initial launch screen: Zowi image/title, Continue / Quit pills, a row of language
> pills with flags (ES/CA/EN/FR/BG), a "no connection available" banner when neither
> Bluetooth nor USB is present, and a dev-only "reset" (forget Zowi) button.

- **File:** `src/views/screens/SplashScreen.qml`
- **i18n context:** `"SplashScreen.qml"`
- **Initial item** of the `StackView` in `main.qml`. It is also reached from
  **Settings → Language** (the whole stack is replaced with it); picking a language
  there and pressing Continue goes back to Home.

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

Same look and structure as [WelcomeScreen](SCREEN_WELCOME.md): **one compact group,
centred** in the window, in a `ColumnLayout` with an equal flexible gap above and
below. Top to bottom: Zowi image, "ZOWI", "Desktop", the Continue / Quit pills, the
language pills and, when needed, the "no connection" notice. The notice is part of
the group (it never covers anything and the group stays centred).

- **Image:** the only elastic piece; it is big (up to `unit × 0.44`, 264 px at
  1024×600) and shrinks first when the window is short (minimum 40 px). It is loaded
  at its source size (960×960), so it stays sharp with fractional scaling.
- **Pills:** Continue is the filled accent pill, Quit (only if
  `button_quit_visible`) the outlined one, as in the Welcome but smaller (≈ 154×45 at
  1024×600). They sit side by side, or stacked when the window is narrow.
- **Language:** a row of five pills, **flag + name** (Español, Valencià, English,
  Français, Български), with the current one highlighted. It wraps to two rows when
  the window is narrow. Flags are drawn in code by `FlagIcon.qml` (no image files).
- **Scale:** `unit = min(width, height)`; title `clamp(unit × 0.06, 26, 64)`
  (36 px at 600), tagline `clamp(unit × 0.027, 13, 24)`, gaps, margins and pills
  scale between sane limits.
- **Wayland (LliureX/KDE):** everything is in logical pixels and there is **no
  popup** (the language selector is always visible), so nothing depends on window
  positions. Compositors ignore client-side window `x`/`y`, so `main.qml` only uses
  them as a hint. `main.qml` also sets `minimumWidth: 480` / `minimumHeight: 360`
  (honoured on Wayland) and sizes the window from the screen it is on
  (`Screen.width/height` × `window_size_ratio`, default 0.75) instead of the whole
  virtual desktop.
- `screenName: "SplashScreen"` lives on the root item (the window title in
  `main.qml` reads it).
- Enter / Return also continues.

## Implementation notes

- **No window reload on a language change.** `main.cpp` used to reload the whole QML
  on `languageChanged`, which destroyed and recreated the window. That reload is gone:
  the Splash (`_rev` read by `tr()`) and `AchievementUnlockDialog` (created once with
  `main.qml`) re-evaluate their texts in place, and any screen created later already
  reads the new language. The language is only changed here, so nothing else is alive
  with stale texts. If another place ever changes the language, its screens must do
  the same (or call for a reload). Debug builds still hot-reload on QML file changes.
- The language pills show the current locale (`Translator.currentLocale()`, kept
  in sync through `languageChanged`) and persist the choice via
  `Session.saveString("locale", ...)`. They replaced the old `ComboBox` + popup, which
  needed a `Column` + `Repeater` workaround on Qt 6.9 (LliureX 25) and pinned palette
  roles to dodge the KDE platform theme.
- The whole app uses the bundled **Noto Sans** (`fonts/`, loaded in `main.cpp`), so
  Valencià / Français / Български render the same on every system.
- `Component.onCompleted: splashScope.forceActiveFocus()`.
- A notice warns when no transport is available
  (`noticeVisible` = `!Robot.bluetoothAvailable && !Robot.usbAvailable`; a property so
  previews can force it).
- Dev reset only accessible when `Config.devMode && Config.devOverlayVisible`
  (overlay with a high `z`).
