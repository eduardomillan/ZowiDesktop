# Migration Qt5 → Qt6

This file documents every change made to migrate from Qt 5.15 to Qt 6.5.2.

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

| File | Change |
|------|--------|
| `CMakeLists.txt` | `find_package(Qt5 ...)` → `find_package(Qt6 ...)` |
| `CMakeLists.txt` | `qt5_add_resources()` → `qt6_add_resources()` |
| `CMakeLists.txt` | `Qt5::Core` / `Qt5::Quick` / `Qt5::Bluetooth` → `Qt6::*` |
| `CMakeLists.txt` | Removed cross-compiling `moc`/`rcc` workaround (Qt6 ships native tools) |
| `src/tests/CMakeLists.txt` | `Qt5::Test` → `Qt6::Test` |

## QML import versions

| File(s) | Qt5 | Qt6 |
|---------|-----|-----|
| `src/views/main.qml` | `QtQuick 2.15` → `QtQuick 6.0` |
| | `QtQuick.Controls 2.15` → `QtQuick.Controls 6.0` |
| | `QtQuick.Window 2.15` → `QtQuick.Window 6.0` |
| `src/views/screens/*.qml` | `QtQuick 2.15` → `QtQuick 6.0` |
| | `QtQuick.Controls 2.15` → `QtQuick.Controls 6.0` |
| `src/views/components/AnimatedZowi.qml` | `QtQuick 2.15` → `QtQuick 6.0` |

## C++ API changes

### Bluetooth signals renamed

In Qt6 `QBluetoothSocket::error` and `QBluetoothDeviceDiscoveryAgent::error` were renamed to `errorOccurred`.

| File | Qt5 | Qt6 |
|------|-----|-----|
| `src/services/BluetoothService.cpp` | `QOverload<...>::of(&QBluetoothSocket::error)` | `&QBluetoothSocket::errorOccurred` |
| `src/services/BluetoothService.cpp` | `QOverload<...>::of(&QBluetoothDeviceDiscoveryAgent::error)` | `&QBluetoothDeviceDiscoveryAgent::errorOccurred` |
| `src/bt_test.cpp` | `QOverload<...>::of(&QBluetoothDeviceDiscoveryAgent::error)` | `&QBluetoothDeviceDiscoveryAgent::errorOccurred` |

## Unchanged (compatible API)

- `QBluetoothSocket::connectToService(QBluetoothAddress, QBluetoothUuid)` — same signature
- `QBluetoothDeviceDiscoveryAgent::start(ClassicMethod)` — same
- `QQmlApplicationEngine`, `QQmlContext`, `Q_PROPERTY`, `Q_INVOKABLE` — same
- `QSettings`, `QXmlStreamReader`, `QTimer` — same
- CMake `CMAKE_AUTOMOC` — still works with Qt6
