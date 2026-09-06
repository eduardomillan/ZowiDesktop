# Architecture

## Table of contents

- [Overview](#overview)
- [Layer Responsibilities](#layer-responsibilities)
  - [zowi_core (pure C++)](#zowi_core-pure-c)
  - [zowi_bt_qt (Qt Bluetooth backend)](#zowi_bt_qt-qt-bluetooth-backend)
  - [src/gui/ (Qt 6 / QML frontend)](#srcgui-qt-6--qml-frontend)
  - [src/cli/ (terminal tool)](#srccli-terminal-tool)
- [Directory Layout](#directory-layout)
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

| Class | Responsibility |
|-------|---------------|
| `DeviceInfo` | Data struct: device name, address, RSSI |
| `SessionStore` | JSON key-value persistence (read/write/get/set) |
| `ConfigStore` | JSON config reader (load from file or string) |
| `TranslationEngine` | JSON-file i18n engine; loader + log callback injectable (Qt-free) |
| `RobotCommands` | Firmware command builder (20 movements) |
| `RobotState` | Cached robot identity/battery state |
| `MessageParser` | Parses incoming robot stream messages |
| `MovementSequencer` | Drives sequences of timed movements |
| `CalibrationSession` | Servo trim calibration state machine |
| `BluetoothApi` | Abstract Bluetooth interface (std::function callbacks) |

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
| `RobotController` | `QtBluetoothBackend` **or** `SerialBluetoothBackend` | `Bluetooth` |
| `ConfigController` | `ConfigStore` | `Config` |
| `CommandsController` | `RobotCommands` + serial command queue | `Commands` |
| `CalibrationSessionController` | `CalibrationSession` | `Calibration` |

`RobotController` is transport-agnostic: it builds either the Qt/BlueZ SPP
backend (`QtBluetoothBackend`) or the serial/USB backend
(`SerialBluetoothBackend`) depending on the selected transport
(`Automatic` / `Bluetooth` / `USB`, chosen from the Settings screen). In
`Automatic` mode it auto-detects the best available transport at startup —
preferring USB when a robot is identified on a serial port via a lightweight
`I` (program-id) handshake — and polls for USB/Bluetooth hotplug. The chosen
transport is persisted in the session store (`transport` key).

### src/cli/ (terminal tool)

CLI11-based tool. Directly instantiates core classes. No QML involved.

| Subcommand | Core class used |
|-----------|----------------|
| `session get/set/list` | `SessionStore` |
| `config get/list` | `ConfigStore` |
| `translate` | `TranslationEngine` |
| `scan` | `QtBluetoothBackend` |

## Directory Layout

```
src/
├── core/                            # Pure C++ library (no Qt, C++20)
│   ├── CMakeLists.txt
│   ├── include/zowi/
│   │   ├── bluetooth_api.h          # Abstract BT interface
│   │   ├── calibration_session.h    # Servo trim calibration
│   │   ├── config_store.h           # JSON config reader
│   │   ├── device_info.h            # Device data struct
│   │   ├── message_parser.h         # Robot stream message parser
│   │   ├── movement_sequencer.h     # Timed movement sequences
│   │   ├── protocol.h               # Firmware framing (&&cmd value%%)
│   │   ├── robot_commands.h         # Firmware command builder
│   │   ├── robot_state.h            # Robot identity/battery state
│   │   ├── session_store.h          # JSON key-value store
│   │   ├── translation_engine.h     # i18n engine (injectable loader)
│   │   └── transport_constants.h    # usb/bt transport identifiers
│   ├── src/
│   │   ├── calibration_session.cpp
│   │   ├── config_store.cpp
│   │   ├── message_parser.cpp
│   │   ├── movement_sequencer.cpp
│   │   ├── robot_commands.cpp
│   │   ├── robot_state.cpp
│   │   ├── session_store.cpp
│   │   └── translation_engine.cpp
│   └── tests/
│       ├── test_calibration_session.cpp
│       ├── test_config_store.cpp
│       ├── test_message_parser.cpp
│       ├── test_movement_sequencer.cpp
│       ├── test_robot_commands.cpp
│       ├── test_robot_state.cpp
│       ├── test_session_store.cpp
│       └── test_translation_engine.cpp
├── backends/bt_qt/                  # Qt Bluetooth backend
│   ├── CMakeLists.txt
│   ├── qt_bluetooth_backend.h
│   └── qt_bluetooth_backend.cpp
├── gui/                             # Qt 6 / QML application
│   ├── CMakeLists.txt
│   ├── main.cpp                     # Entry point, hot-reload, context wiring
│   └── controllers/
│       ├── RobotController         # situation state machine, transport
│       ├── SessionController       # SessionStore adapter
│       ├── TranslatorController    # TranslationEngine adapter (Qt loader)
│       ├── ConfigController        # ConfigStore adapter
│       ├── CalibrationSession      # CalibrationSession adapter
│       └── CommandsController      # RobotCommands/serial command queue
├── cli/                             # CLI tool
│   ├── CMakeLists.txt
│   ├── main.cpp                     # entry point
│   ├── cli_commands.cpp/h           # CLI11 subcommand wiring
│   ├── cli_state.cpp/h              # app/bot state helper
│   └── cli_util.cpp/h               # logging/fs helpers
├── views/                           # QML screens
│   ├── main.qml
│   └── screens/
│       ├── HomeScreen.qml
│       ├── ScanScreen.qml
│       ├── SplashScreen.qml
│       ├── WelcomeScreen.qml
│       └── WizardScreen.qml
├── config.json                      # App config (image paths, URLs)
└── i18n/                            # Translation files (.json)
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
| `ZowiDesktop` | Executable | `zowi_core`, `zowi_bt_qt`, `zowi_bt_serial*`, Qt::Quick, Qt::QuickControls2 |
| `zowi_cli` | Executable | `zowi_core`, `zowi_firmware`, CLI11, Qt::Core (+ backend libs) |
| `test_*` | Test exe | `zowi_core` only |

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `ZOWI_BUILD_GUI` | ON | Build Qt 6 GUI |
| `ZOWI_BUILD_CLI` | ON | Build CLI tool |
| `BUILD_TESTS` | ON | Build unit tests |

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
| Qt 6 | 6.5+ | GUI framework, Bluetooth | System install |

## Screen Navigation

Navigation uses a `StackView` in `main.qml`:

```
SplashScreen → WelcomeScreen → WizardScreen → ScanScreen → HomeScreen
                    ↑              ↓
                    └──────────────┘  (dismissed)
```

## View Mapping (Android → Desktop)

| Android Activity | Desktop QML | Status |
|---|---|---|
| `SplashViewActivity` | `SplashScreen.qml` | Done |
| `WelcomeViewActivity` | `WelcomeScreen.qml` | Done |
| `WizardViewActivity` | `WizardScreen.qml` + `ScanScreen.qml` | Done |
| `HomeViewActivity` | `HomeScreen.qml` | Done |
| `SettingsViewActivity` | `SettingsScreen.qml` | Planned |
| `AchievementsViewActivity` | `AchievementsScreen.qml` | Planned |
| `PadViewActivity` | `PadScreen.qml` | Planned |
| `TimelineActivity` | `TimelineScreen.qml` | Planned |

## QML Hot-Reload

In **debug** mode, QML is loaded from the filesystem. A `QFileSystemWatcher` monitors `src/views/` and triggers a full engine reload on any `.qml` change. Language changes also trigger `reloadQml()`.

## Testing

Core library tests run without Qt:

```bash
cmake -B build -DZOWI_BUILD_CLI=OFF -DZOWI_BUILD_GUI=OFF
cmake --build build
ctest --test-dir build
```

8 tests, all Qt-free: `test_session_store`, `test_config_store`,
`test_translation_engine`, `test_robot_commands`, `test_robot_state`,
`test_message_parser`, `test_movement_sequencer`, `test_calibration_session`.
The CLI also ships a black-box test (`cli_blackbox`).

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
