# Architecture

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
| `TranslationEngine` | Custom XML-based i18n engine |
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
| `BluetoothController` | `QtBluetoothBackend` | `Bluetooth` |
| `ConfigController` | `ConfigStore` | `Config` |

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
├── core/                            # Pure C++ library (no Qt)
│   ├── CMakeLists.txt
│   ├── include/zowi/
│   │   ├── bluetooth_api.h          # Abstract BT interface
│   │   ├── config_store.h           # JSON config reader
│   │   ├── device_info.h            # Device data struct
│   │   ├── session_store.h          # JSON key-value store
│   │   └── translation_engine.h     # i18n engine
│   ├── src/
│   │   ├── config_store.cpp
│   │   ├── session_store.cpp
│   │   └── translation_engine.cpp
│   └── tests/
│       ├── test_config_store.cpp
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
│       ├── BluetoothController.h/.cpp
│       ├── ConfigController.h/.cpp
│       ├── SessionController.h/.cpp
│       └── TranslatorController.h/.cpp
├── cli/                             # CLI tool
│   ├── CMakeLists.txt
│   └── main.cpp
├── views/                           # QML screens
│   ├── main.qml
│   └── screens/
│       ├── HomeScreen.qml
│       ├── ScanScreen.qml
│       ├── SplashScreen.qml
│       ├── WelcomeScreen.qml
│       └── WizardScreen.qml
├── config.json                      # App config (image paths, URLs)
└── i18n/                            # Translation files (.ts)
```

## Build Targets

| Target | Type | Depends on |
|--------|------|-----------|
| `zowi_core` | Static lib | nlohmann/json (FetchContent) |
| `zowi_bt_qt` | Static lib | `zowi_core`, Qt6::Core, Qt6::Bluetooth |
| `ZowiDesktop` | Executable | `zowi_core`, `zowi_bt_qt`, Qt6::Quick, Qt6::QuickControls2 |
| `zowi_cli` | Executable | `zowi_core`, `zowi_bt_qt`, CLI11 (FetchContent), Qt6::Core, Qt6::Bluetooth |
| `test_*` | Test exe | `zowi_core` |

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
| CLI11 | v2.4.2 | CLI argument parsing | FetchContent |
| nlohmann/json | v3.11.3 | JSON read/write | FetchContent |
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

3 tests: `test_session_store`, `test_config_store`, `test_translation_engine`.
