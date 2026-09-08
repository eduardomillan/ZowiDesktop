# GUI Screens — Index

> Every QML screen in the GUI lives in `src/views/screens/`. Each entry below
> documents one screen: what it does, how it navigates, the QML context it uses
> and the commands it sends. Screens are the thin adapter layer over the core —
> see [docs/project/ARCHITECTURE.md](../ARCHITECTURE.md) and
> [docs/project/MVVM.md](../MVVM.md) for the overall architecture.

## Screens

| Screen | File | Document |
|--------|------|----------|
| Splash | `src/views/screens/SplashScreen.qml` | [SCREEN_SPLASH.md](SCREEN_SPLASH.md) |
| Welcome | `src/views/screens/WelcomeScreen.qml` | [SCREEN_WELCOME.md](SCREEN_WELCOME.md) |
| Wizard | `src/views/screens/WizardScreen.qml` | [SCREEN_WIZARD.md](SCREEN_WIZARD.md) |
| Wizard found | `src/views/screens/WizardFoundScreen.qml` | [SCREEN_WIZARD_FOUND.md](SCREEN_WIZARD_FOUND.md) |
| Wizard rename | `src/views/screens/WizardRenameScreen.qml` | [SCREEN_WIZARD_RENAME.md](SCREEN_WIZARD_RENAME.md) |
| Scan | `src/views/screens/ScanScreen.qml` | [SCREEN_SCAN.md](SCREEN_SCAN.md) |
| Home | `src/views/screens/HomeScreen.qml` | [SCREEN_HOME.md](SCREEN_HOME.md) |
| Settings | `src/views/screens/SettingsScreen.qml` | [SCREEN_SETTINGS.md](SCREEN_SETTINGS.md) |
| Calibration | `src/views/screens/CalibrationScreen.qml` | [SCREEN_CALIBRATION.md](SCREEN_CALIBRATION.md) |
| Mouth editor (pintabocas) | `src/views/screens/MouthEditorScreen.qml` | [SCREEN_MOUTH_EDITOR.md](SCREEN_MOUTH_EDITOR.md) |
| Pad (gamepad) | `src/views/screens/PadScreen.qml` | [SCREEN_PAD.md](SCREEN_PAD.md) |
| Mouth picker | `src/views/screens/MouthScreen.qml` | [SCREEN_MOUTH.md](SCREEN_MOUTH.md) |
| Gesture picker | `src/views/screens/GestureScreen.qml` | [SCREEN_GESTURE.md](SCREEN_GESTURE.md) |
| Project (generic, all Discover lessons) | `src/views/screens/ProjectScreen.qml` | [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md) |
| Base template (not a screen) | `src/views/screens/ScreenTemplate.qml` | [SCREEN_TEMPLATE.md](SCREEN_TEMPLATE.md) |

## Navigation map

The root window `src/views/main.qml` owns the `StackView` and wires every
transition via signals. Initial item is `SplashScreen`.

```
SplashScreen ──(hasDismissedWizard || hasDevice)──▶ HomeScreen
     │                                                    │
     └──(else)──▶ WelcomeScreen                           ├─▶ SettingsScreen ──▶ CalibrationScreen
                      │                                   ├─▶ MouthEditorScreen
                      └─▶ WizardScreen                    ├─▶ PadScreen ──▶ MouthScreen
                              │                                              └─▶ GestureScreen
                              ├─(USB-only)──▶ WizardFoundScreen(usb)          └─▶ ProjectScreen (any Discover
                              └─(BT)──▶ ScanScreen ──▶ WizardFoundScreen ──▶ (rename if default name)      project, from Projects page)
                                        └─▶ WizardRenameScreen
```

| From | Event | Pushed by `main.qml` |
|------|-------|-----------------------|
| Splash | `splashFinished` | Home (replace) or Welcome (replace) |
| Welcome | `startWizard` | Wizard |
| Wizard | `startClicked` | Scan (BT) or WizardFound with `usbMode` (USB-only) |
| Wizard | `dismissed` | Home (replace, saves `wizardDismissed`) |
| Scan | `deviceSelected` | WizardFound |
| WizardFound | `paired` | `finishRegistration()` → Home (replace) |
| Home | `gamepadClicked` | Pad |
| Pad | `mouthScreenRequested` / `gestureScreenRequested` | Mouth / Gesture |
| Home | `projectRequested(projectId)` | Project (any Discover lesson) |
| Home | `mouthEditorClicked` | MouthEditor |
| Home | `settingsClicked` | Settings |
| Settings | `calibrationRequested` | Calibration |
| Settings | `forgetCompleted` | Welcome (replace) |

The `DevOverlay` is instantiated at window scope and toggled from any screen
with **Ctrl+D** (window-level `Shortcut` in `main.qml`).

## Shared conventions

- **i18n context** = the QML file name: every screen calls
  `Translator.translate("<Screen>.qml", source)`; keys live in
  `i18n/zowi_<locale>.json` (all locales: `es_ES`, `ca_ES`, `en_US`, `fr_FR`,
  `bg_BG`).
- **Context objects** registered in `src/gui/main.cpp`: `Session`, `Translator`,
  `Robot`, `Config`, `Calibration`, `Commands`, `AppVersion`.
- **Back button** comes from `ScreenTemplate` (`backClicked` signal).
- **Command builders** are the `Commands` object (`CommandsController`), which
  wraps the Qt-free `zowi::robot_commands` core module.
- **Data polling**: screens that stream commands continuously
  (Calibration, MouthEditor, Pad while a button is held) pause the robot
  identity poll via `Robot.setDataPollingEnabled(false)` and re-enable it on
  destruction, so the `E/I/B` identity burst never interrupts the queue.

## Related

- Previewing a single screen: `zowi_screen_preview <screen.qml>`
  (see [docs/project/BUILD.md](../BUILD.md) — `PREVIEW_GRAB_DIR`).
- Translation keys and contexts: `zowi_cli translate -c "<Screen>.qml" -s "<text>"`
  (see [docs/project/ZOWI_CLI_HOWTO.md](../ZOWI_CLI_HOWTO.md)).