# Architecture

## Table of contents

- [Overview](#overview)
- [Layer Responsibilities](#layer-responsibilities)
  - [zowi_core (pure C++)](#zowi_core-pure-c)
  - [zowi_bt_qt (Qt Bluetooth backend)](#zowi_bt_qt-qt-bluetooth-backend)
  - [src/gui/ (Qt 6 / QML frontend)](#srcgui-qt-6--qml-frontend)
  - [src/cli/ (terminal tool)](#srccli-terminal-tool)
- [Directory Layout](#directory-layout)
- [Resources and fonts](#resources-and-fonts)
- [Build Targets](#build-targets)
  - [CMake options](#cmake-options)
- [Data Flow](#data-flow)
  - [GUI path](#gui-path)
  - [CLI path](#cli-path)
- [Third-party Dependencies](#third-party-dependencies)
- [Screen Navigation](#screen-navigation)
- [View Mapping (Android → Desktop)](#view-mapping-android--desktop)
- [QML Hot-Reload](#qml-hot-reload)
- [Testing](#testing)
- [Architecture evolution (MVVM → API + CLI + Frontend)](#architecture-evolution-mvvm--api--cli--frontend)

## Overview

Zowi Desktop follows an **API + CLI + Frontend** architecture. Business logic lives in a pure C++ core library with no Qt dependency. Two consumers use this core independently:

- **CLI** (`zowi_cli`) — terminal tool for debugging and scripting
- **GUI** (`ZowiDesktop`) — Qt 6 / QML desktop application

```
┌─────────────────────────────────────────────────────┐
│                     Consumers                       │
│                                                     │
│  ┌──────────────┐              ┌──────────────────┐ │
│  │  zowi_cli    │              │  ZowiDesktop     │ │
│  │  (CLI11)     │              │  (Qt 6 / QML)    │ │
│  └──────┬───────┘              └────────┬─────────┘ │
│         │                               │           │
│         │         ┌─────────┐           │           │
│         └────────►│zowi_core│◄──────────┘           │
│                   │(C++ 20) │                       │
│                   │no Qt dep│                       │
│                   └────┬────┘                       │
│                        │                            │
│              ┌─────────▼──────────┐                 │
│              │   zowi_bt_qt       │                 │
│              │ (Qt Bluetooth)     │                 │
│              └────────────────────┘                 │
└─────────────────────────────────────────────────────┘
```

## Layer Responsibilities

### zowi_core (pure C++)

Core business logic. Zero Qt dependency. Testable on any platform.

| Class / module | Responsibility |
|----------------|---------------|
| `DeviceInfo` | Data struct: device name, address, RSSI |
| `SessionStore` | JSON key-value persistence (read/write/get/set); config directory injectable |
| `ConfigStore` | JSON config reader (file or string) plus the override layers (`applyOverrides()`: system file > `ZOWI_<KEY>` environment > user file > compiled defaults; see `CONFIG_HOWTO.md`) |
| `TranslationEngine` | JSON-file i18n engine; loader + log callback injectable (Qt-free) |
| `protocol.h` | Firmware framing (`&&cmd value%%`), `makeCommand()`, `isValidRobotName()` / `kMaxRobotNameBytes` (letters only, 10 bytes) |
| `RobotCommands` | Firmware command builder (movements, gestures, mouths, sounds, trims) |
| `RobotState` | Cached robot identity/battery state |
| `MessageParser` | Parses incoming robot stream messages |
| `MovementSequencer` | Drives sequences of timed movements |
| `CalibrationSession` | Servo trim calibration state machine |
| `ProjectsStore`, `project_model.h` | Discover projects: metadata and localized pages/quizzes (see `PROJECTS_HOWTO.md`) |
| `ProjectsPreferencesStore` | Project preferences: quiz lock duration, quiz/achievements switches and per-project overrides |
| `ZowiDiceGame` | Logic of the Memory game ("Zowi says"): the random action sequence and the rounds |
| `MouthsGame` | Logic of the Draw-the-mouths game: rounds, levels and matching |
| `TimelineCommand`, `TimelinePlayer`, `timelineScore()` | Timeline chips, their step-by-step playback paced by the robot's acks, and the score of a completed sequence |
| `AchievementsStore`, `achievements.h` | The 17 achievements, their persistence (own `ZowiAchievements.json`), usage counters and the pure unlock rules (see `ACHIEVEMENTS_HOWTO.md`) |
| `RankingStore` | Global ranking of local players (`Player-NNN`) in its own `ZowiRanking.json` (see `RANKING_HOWTO.md`) |
| `ranking_online.h` | Parsing/validation of the public world ranking JSON (no network code) |
| `transport_constants.h` | `usb` / `bt` transport identifiers stored in the session |
| `BluetoothApi` | Abstract Bluetooth/transport interface (std::function callbacks) |

### zowi_bt_qt (Qt Bluetooth backend)

Implements `BluetoothApi` using Qt Bluetooth. Bridges core's abstract interface to the platform's Bluetooth stack.

- `QtBluetoothBackend` — discovery, SPP connection, data send/receive
- Reconnect timer on disconnection
- Used by both CLI and GUI

### src/gui/ (Qt 6 / QML frontend)

Qt Quick application. Controllers wrap core classes and expose them to QML via context properties.

| Controller | Wraps | QML context |
|-----------|-------|-------------|
| `SessionController` | `SessionStore` | `Session` |
| `TranslatorController` | `TranslationEngine` | `Translator` |
| `RobotController` | `QtBluetoothBackend` / `NativeBluetoothBackend` **or** the serial/USB backend | `Robot` |
| `ConfigController` | `ConfigStore` (with the override layers) | `Config` |
| `CommandsController` | `RobotCommands` + serial command queue | `Commands` |
| `CalibrationSessionController` | `CalibrationSession` | `Calibration` |
| `ProjectsController` | `ProjectsStore` + `ProjectsPreferencesStore` | `Projects` |
| `ZowiDiceController` | `ZowiDiceGame` (Memory game) | `ZowiDice` |
| `MouthsGameController` | `MouthsGame` (Draw the mouths) | `Mouths` |
| `TimelineController` | `TimelinePlayer`, `timelineScore()` | `Timeline` |
| `RankingController` | `RankingStore` (read, create/switch players, record scores; no delete) | `Ranking` |
| `AchievementsController` | `AchievementsStore` (unlock events, thresholds, locked-content queries, notice/celebration; the layer is switched by the `achievements_*` config keys) | `Achievements` |
| `OnlineRankingClient` | `ranking_online` + `QNetworkAccessManager` (world ranking read/share/delete) | `OnlineRanking` |

`main.cpp` also exposes the plain string `AppVersion`.

`RobotController` is transport-agnostic: it builds the Bluetooth backend of the
platform (`QtBluetoothBackend` on Linux, `NativeBluetoothBackend` on Windows) or the
serial/USB backend. The transport is **not chosen by the user**: a *situation* state
machine (`Demo` / `Unregistered` / `Connecting` / `Connected` / `Disconnected` /
`TransportLost`) is derived from the availability of USB and Bluetooth, whether a Zowi
is registered and the connection state; the registered Zowi's transport
(`activeZowiTransport`, `usb`/`bt`) wins, and Settings only shows the status and the
actions that make sense (see `TRANSPORT_HOWTO.md`). Bluetooth availability is probed on
a background thread so a machine without Bluetooth never stalls the start-up.
`RobotController` also owns the battery policy: one threshold (`low_battery_threshold`)
drives `batteryLow`, the one-off `lowBatteryDetected` signal and the confirmation before
installing firmware.

### src/cli/ (terminal tool)

CLI11-based tool. Directly instantiates core classes. No QML involved.

| Subcommand | Core class used |
|-----------|----------------|
| `session get/set/clear/list` | `SessionStore` |
| `config get/list` | `ConfigStore` |
| `ranking list/clear` | `RankingStore` (admin only; the GUI has no delete) |
| `achievements list/unlock/reset` | `AchievementsStore` (admin: inspect, unlock or reset the progress) |
| `translate` | `TranslationEngine` |
| `ports`, `scan` | serial port listing, `QtBluetoothBackend` |
| `connect`, `disconnect`, `status`, `rename` | transport backends, `SessionStore`, `protocol.h` |
| `restore`, `alarm`, `adivinawi` | `zowi_firmware` (STK500v1) over the transport |
| `control`, `move`, `gesture`, `mouth`, `sing`, `calibrate` | `RobotCommands`, `CalibrationSession` |
| `shell` | interactive REPL over the same commands |

Full reference: `ZOWI_CLI_HOWTO.md` and `ZOWI_CLI_SHELL.md`.

## Directory Layout

```
.
├── VERSION                          # single source of truth for the version
├── CMakeLists.txt, build.sh, build.bat, run-tests.sh
├── app.qrc, i18n.qrc                # hand-maintained resource files
├── src/
│   ├── core/                        # Pure C++ library (no Qt, C++20)
│   │   ├── include/zowi/            # public headers (see the core table above)
│   │   ├── src/                     # implementations
│   │   └── tests/                   # test_*.cpp, link only Zowi::core
│   ├── firmware/                    # zowi_firmware: STK500v1 / raw-HEX uploader + the .hex files
│   ├── backends/
│   │   ├── bt_qt/                   # Qt Bluetooth (BlueZ SPP), Linux
│   │   ├── bt_native/               # WinRT Bluetooth, Windows
│   │   ├── bt_serial/               # POSIX serial / USB
│   │   └── bt_serial_win/           # Win32 serial / USB
│   ├── gui/                         # Qt 6 / QML application
│   │   ├── main.cpp                 # entry point, fonts, hot-reload, context wiring
│   │   ├── preview_main.cpp         # zowi_screen_preview (renders one screen headless)
│   │   └── controllers/             # thin Qt adapters, one per QML context property
│   ├── cli/                         # zowi_cli (CLI11) + tests/ (black-box, bt, usb)
│   ├── views/                       # QML
│   │   ├── main.qml                 # StackView, navigation wiring, global notice
│   │   ├── screens/                 # one file per screen (see screens/INDEX.md)
│   │   └── components/              # StatusBar, MessageBar, RankingDialog, …
│   └── config.json                  # compiled-in defaults (see CONFIG_HOWTO.md)
├── i18n/                            # zowi_<locale>.json translation files
├── images/                          # app images (embedded) and app icons
├── fonts/                           # bundled Noto Sans + OFL license
├── projects/                        # Discover projects data (JSON, localized HTML, quizzes)
├── server/ranking-worker/           # Cloudflare Worker for the world ranking
├── packaging/                       # AppImage, .deb, Windows installer/zip, release scripts
├── scripts/                         # helper scripts (firmware sync, BT capability)
└── docs/                            # project docs (docs/project), tests docs, firmware docs
```

## Build Targets

| Target | Type | Depends on |
|--------|------|-----------|
| `zowi_core` | Static lib | nlohmann/json |
| `zowi_firmware` | Static lib (STK500v1/Optiboot) | — |
| `zowi_bt_qt` | Static lib | `zowi_core`, Qt::Core, Qt::Bluetooth, Qt::DBus |
| `zowi_bt_serial` | Static lib (POSIX serial) | `zowi_core`, Qt::Core |
| `zowi_bt_serial_win` | Static lib (Win32 serial) | `zowi_core`, Qt::Core |
| `zowi_bt_native` | Static lib (Windows WinRT) | `zowi_core` (+ Qt::Core, WinRT privately) |
| `ZowiDesktop` | Executable | `zowi_core`, `zowi_firmware`, `zowi_bt_qt`/`zowi_bt_native`, `zowi_bt_serial*`, Qt::Quick, Qt::QuickControls2, Qt::Network |
| `zowi_screen_preview` | Executable | same as the GUI; renders a single screen (optionally to a PNG with `PREVIEW_GRAB_DIR`) for layout work without a display |
| `zowi_cli` | Executable | `zowi_core`, `zowi_firmware`, CLI11, Qt::Core (+ backend libs) |
| `test_*` | Test exe | `zowi_core` only |

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `ZOWI_BUILD_GUI` | ON | Build Qt 6 GUI |
| `ZOWI_BUILD_CLI` | ON | Build CLI tool |
| `BUILD_TESTS` | ON | Build unit tests |
| `ZOWI_QT_VERSION` | 6 | Qt major version to build against (5 or 6; `./build.sh -5` selects Qt 5.15, see `DOWNGRADING_TO_QT5.md`) |
| `FETCHLIBRARIES` | ON | Fetch nlohmann_json and CLI11 from GitHub when they are not installed |

## Resources and fonts

The GUI embeds its assets as Qt resources. `views.qrc`, `images.qrc`, `projects.qrc` and
`fonts.qrc` are **generated by CMake at configure time** (`cmake/GenerateQrc.cmake`,
globbing `src/views/`, `images/`, `projects/` and `fonts/`), so adding a QML file, an
image, a project file or a font only needs a rebuild; `app.qrc` (config, firmware hex,
icon) and `i18n.qrc` are maintained by hand. Debug builds load the QML from disk (hot
reload); release builds load everything from the resources.

The application font is the bundled **Noto Sans** (`fonts/`, SIL OFL 1.1), registered and
set app-wide in `main.cpp`, so accents and Cyrillic render the same on every system. Only
the DEV overlay and the mouth bit-pattern use `monospace`.

## Data Flow

### GUI path

```
QML (View) ←→ Controller (Qt adapter) ←→ Core class ←→ QtBluetoothBackend
     │                  │                    │
     │  Q_INVOKABLE     │  wraps             │  std::function callbacks
     │  Q_PROPERTY       │                    │
     └──────────────────┘                    │
                                             ▼
                                      Qt Bluetooth stack
```

### CLI path

```
CLI11 args → main.cpp → Core class → QtBluetoothBackend
                                        │
                                        ▼
                                 Qt Bluetooth stack
```

## Third-party Dependencies

| Library | Version | Purpose | How obtained |
|---------|---------|---------|-------------|
| CLI11 | v2.4.2 | CLI argument parsing | System package, or FetchContent fallback |
| nlohmann/json | v3.11.3 | JSON read/write | System package, or FetchContent fallback (`-DFETCHLIBRARIES=TRUE`) |
| Qt 6 | 6.5+ (target platform LliureX 25: 6.9) | GUI framework, Bluetooth, network | System install |

## Screen Navigation

Navigation uses a `StackView` in `main.qml`; every screen only emits signals and
`main.qml` decides where to go (details and the full signal table in
[screens/INDEX.md](screens/INDEX.md)):

```
SplashScreen ──(wizard dismissed or Zowi registered)──▶ HomeScreen
     │                                                     ├─▶ Pad / Timeline / Memory / Mouths / MouthEditor
     └──(else)──▶ WelcomeScreen ──▶ WizardScreen           ├─▶ ProjectScreen (any Discover lesson)
                                  └─▶ ScanScreen ──▶       └─▶ SettingsScreen ──▶ Calibration / WizardRename
                                      WizardFoundScreen ──▶ Home          └─(Language)──▶ SplashScreen
```

Settings → Language replaces the whole stack with the splash screen; the language is
picked there and "Continue" goes back to Home.

## View Mapping (Android → Desktop)

| Android Activity | Desktop QML | Status |
|---|---|---|
| `SplashViewActivity` | `SplashScreen.qml` | Done |
| `WelcomeViewActivity` | `WelcomeScreen.qml` | Done |
| `WizardViewActivity` | `WizardScreen.qml`, `ScanScreen.qml`, `WizardFoundScreen.qml`, `WizardRenameScreen.qml` | Done |
| `HomeViewActivity` | `HomeScreen.qml` | Done (plus a ranking button) |
| `SettingsViewActivity` | `SettingsScreen.qml` | Done (plus Language, which the original lacks) |
| `PadViewActivity` | `PadScreen.qml` (+ gesture/mouth/sounds screens, calibration) | Done |
| `TimelineActivity` | `GameTimelineScreen.qml` | Done |
| Zowi says / Mouths minigames | `GameMemoryScreen.qml`, `GameMouthsScreen.qml` | Done |
| `ProjectViewActivity`, `ProjectQuizViewActivity` | `ProjectScreen.qml` + `QuizComponent.qml` | Done |
| `AchievementsViewActivity` | `AchievementsScreen.qml` + `AchievementUnlockDialog.qml` | Done |

## QML Hot-Reload

In **debug** mode, QML is loaded from the filesystem. A `QFileSystemWatcher` monitors `src/views/` and triggers a full engine reload on any `.qml` change. Language changes also trigger `reloadQml()`.

## Testing

Core library tests run without Qt:

```bash
cmake -B build -DZOWI_BUILD_CLI=OFF -DZOWI_BUILD_GUI=OFF
cmake --build build
ctest --test-dir build
```

17 tests, all Qt-free (`ctest --test-dir build -N` lists them): `test_session_store`,
`test_config_store`, `test_translation_engine`, `test_robot_commands`,
`test_calibration_session`, `test_message_parser`, `test_movement_sequencer`,
`test_robot_state`, `test_projects_store`, `test_zowi_dice`, `test_mouths_game`,
`test_timeline_command`, `test_timeline_player`, `test_timeline_score`,
`test_ranking_store`, `test_ranking_online` and `test_achievements`. The CLI also ships a black-box test
(`cli_blackbox`). The Cloudflare Worker has its own tests:
`node --test server/ranking-worker/test/ranking.test.js`.

## Architecture evolution (MVVM → API + CLI + Frontend)

The project originally used a monolithic **MVVM** pattern tightly coupled to Qt:
business logic lived in `QObject` subclasses (`SessionService`,
`BluetoothService`, `TranslationEngine`), every service required Qt, testing
needed the full Qt stack (QTest + QML context), there was no CLI, and the
Bluetooth logic was coupled to QML, making it hard to swap backends (e.g. a
native Win32 backend).

The migration to the current **API + CLI + Frontend** layout replaced the Qt
services with a pure C++ core (`src/core/`, no Qt dependency) exposed through
the abstract `BluetoothApi`, added a pluggable backend layer
(`src/backends/`), a CLI consumer (`zowi_cli`) for scripting/debugging, and Qt
adapters (`src/gui/controllers/`) that merely expose core classes to QML.

| Was (MVVM) | Now (API + CLI + Frontend) |
|---|---|
| `src/services/*` (Qt `QObject` services) | `src/core/` — pure C++; single CLI entry |
| `src/controllers/*` (ViewModel layer) | `src/gui/controllers/` — thin Qt adapters |
| `src/main.cpp` (single entry point) | `src/gui/main.cpp` + `src/cli/main.cpp` |
| `src/BtTest.cpp` | `zowi_cli scan` |
| `src/tests/*` (QTest, Qt-coupled) | `src/core/tests/` — run without Qt |
| `zowi_*_legacy` CMake targets | removed |

Benefits:

- **Testable without Qt** — core tests compile and run without Qt installed.
- **CLI for debugging** — `zowi_cli` covers session, config, translate, scan,
  and robot commands without launching the GUI.
- **Pluggable backends** — `BluetoothApi` is transport-agnostic; `bt_qt`,
  `bt_native`, `bt_serial` and `bt_serial_win` can be swapped without touching
  core code, and a mock can be used in tests.
- **Fast iteration** — core changes recompile only `zowi_core`; CLI changes
  recompile only `zowi_cli`; QML changes hot-reload with no C++ rebuild.
- **Independent consumers** — CLI and GUI build separately
  (`-DZOWI_BUILD_GUI=OFF` / `-DZOWI_BUILD_CLI=OFF`).
