# Upgrading to Qt 6

This document documents the migration from Qt 5.15 to Qt 6.5.2, including changes required by the current API + CLI + Frontend architecture.

## Scope

Qt is used in two places only:

- `src/backends/bt_qt/` — Bluetooth backend (implements `BluetoothApi`)
- `src/gui/` — Qt Quick application

The core library (`src/core/`) and CLI (`src/cli/`) have no Qt dependency (except the CLI's `scan` subcommand, which uses `QtBluetoothBackend`).

## Dependencies

| Qt5 | Qt6 |
|-----|-----|
| `qtbase5-dev` | `qt6-base-dev` |
| `qtdeclarative5-dev` | `qt6-declarative-dev` |
| _not available_ | `qt6-connectivity-dev` (Bluetooth) |
| `libqt5xml5` | _built into qt6-base-dev_ |

```bash
# Lliurex 23 / Ubuntu Jammy
sudo apt install qt6-base-dev qt6-declarative-dev qt6-connectivity-dev
```

## CMake changes

### Root CMakeLists.txt

| Change |
|--------|
| `find_package(Qt5 ...)` → `find_package(Qt6 ...)` |
| C++ standard bumped from 17 to 20 |

### src/backends/bt_qt/CMakeLists.txt

| Qt5 | Qt6 |
|-----|-----|
| `Qt5::Core Qt5::Bluetooth` | `Qt6::Core Qt6::Bluetooth` |

### src/gui/CMakeLists.txt

| Qt5 | Qt6 |
|-----|-----|
| `qt5_add_resources()` | `qt6_add_resources()` |
| `Qt5::Core Qt5::Quick Qt5::QuickControls2` | `Qt6::Core Qt6::Quick Qt6::QuickControls2` |

## QML import versions

| File(s) | Qt5 | Qt6 |
|---------|-----|-----|
| `src/views/main.qml` | `QtQuick 2.15` | `QtQuick 6.0` |
| | `QtQuick.Controls 2.15` | `QtQuick.Controls 6.0` |
| | `QtQuick.Window 2.15` | `QtQuick.Window 6.0` |
| `src/views/screens/*.qml` | `QtQuick 2.15` | `QtQuick 6.0` |
| | `QtQuick.Controls 2.15` | `QtQuick.Controls 6.0` |
| `src/views/components/AnimatedZowi.qml` | `QtQuick 2.15` | `QtQuick 6.0` |

## C++ API changes

### Bluetooth signals renamed

In Qt6, `QBluetoothSocket::error` and `QBluetoothDeviceDiscoveryAgent::error` were renamed to `errorOccurred`.

| File | Qt5 | Qt6 |
|------|-----|-----|
| `src/backends/bt_qt/qt_bluetooth_backend.cpp` | `QOverload<...>::of(&QBluetoothSocket::error)` | `&QBluetoothSocket::errorOccurred` |
| `src/backends/bt_qt/qt_bluetooth_backend.cpp` | `QOverload<...>::of(&QBluetoothDeviceDiscoveryAgent::error)` | `&QBluetoothDeviceDiscoveryAgent::errorOccurred` |

## Unchanged (compatible API)

- `QBluetoothSocket::connectToService(QBluetoothAddress, QBluetoothUuid)` — same signature
- `QBluetoothDeviceDiscoveryAgent::start(ClassicMethod)` — same
- `QQmlApplicationEngine`, `QQmlContext`, `Q_PROPERTY`, `Q_INVOKABLE` — same
- `QTimer` — same
- CMake `CMAKE_AUTOMOC` — still works with Qt6

## Architecture impact

The API + CLI + Frontend architecture isolates Qt to two layers:

- `zowi_core` — no Qt dependency (pure C++20)
- `zowi_cli` — uses Qt only for the `scan` subcommand (QtBluetoothBackend)
- `zowi_bt_qt` — Qt Bluetooth implementation
- `ZowiDesktop` — Qt Quick GUI

If Qt6 causes issues, only `zowi_bt_qt` and `src/gui/` need changes. The core library and CLI's non-BT commands are unaffected.
