# Downgrading to Qt 5

This document describes the changes needed to build Zowi Desktop with Qt 5.15 instead of Qt 6.5.

## Scope

Qt is used in two places only:

- `src/backends/bt_qt/` — Bluetooth backend (implements `BluetoothApi`)
- `src/gui/` — Qt Quick application

The core library (`src/core/`) has no Qt dependency and needs no changes. The CLI (`src/cli/`) uses Qt only for the `scan` subcommand.

## Dependencies

| Qt6 | Qt5 |
|-----|-----|
| `qt6-base-dev` | `qtbase5-dev` |
| `qt6-declarative-dev` | `qtdeclarative5-dev` |
| `qt6-connectivity-dev` | `libqt5 bluetooth5-dev` (or `qtconnectivity5-dev`) |
| `libqt6xml6` | `libqt5xml5` |

```bash
# Debian/Ubuntu
sudo apt install qtbase5-dev qtdeclarative5-dev qtconnectivity5-dev
```

## CMake changes

### Root CMakeLists.txt

```cmake
# Qt6 → Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Bluetooth)

# C++ standard: Qt5 works best with C++17
set(CMAKE_CXX_STANDARD 17)
```

### src/backends/bt_qt/CMakeLists.txt

```cmake
# Qt6::Core Qt6::Bluetooth → Qt5::Core Qt5::Bluetooth
target_link_libraries(zowi_bt_qt
    PUBLIC  Zowi::core Qt5::Core Qt5::Bluetooth
)
```

### src/gui/CMakeLists.txt

```cmake
# qt6_add_resources → qt5_add_resources
qt5_add_resources(GUI_RESOURCES ${CMAKE_SOURCE_DIR}/resources.qrc)

# Qt6::* → Qt5::*
target_link_libraries(ZowiDesktop PRIVATE
    Zowi::core
    Zowi::bt_qt
    Qt5::Core
    Qt5::Quick
    Qt5::QuickControls2
)
```

### src/cli/CMakeLists.txt

```cmake
# Qt6::Bluetooth → Qt5::Bluetooth
target_link_libraries(zowi_cli PRIVATE
    Zowi::core
    Zowi::bt_qt
    Qt5::Bluetooth
    CLI11::CLI11
)
```

## C++ API changes

### Bluetooth signals

Qt6 renamed `error` to `errorOccurred`. In Qt5, use the old name with `QOverload`:

**File:** `src/backends/bt_qt/qt_bluetooth_backend.cpp`

```cpp
// Qt6 (current)
QObject::connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, ...);
QObject::connect(m_socket, &QBluetoothSocket::errorOccurred, ...);

// Qt5 (downgrade)
QObject::connect(m_discoveryAgent,
    QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
    this, &QtBluetoothBackend::onScanError);

QObject::connect(m_socket,
    QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
    this, &QtBluetoothBackend::onSocketError);
```

## QML import versions

All `.qml` files need import version downgrades:

| Qt6 | Qt5 |
|-----|-----|
| `import QtQuick 6.0` | `import QtQuick 2.15` |
| `import QtQuick.Controls 6.0` | `import QtQuick.Controls 2.15` |
| `import QtQuick.Window 6.0` | `import QtQuick.Window 2.15` |

**Files affected:**
- `src/views/main.qml`
- `src/views/screens/SplashScreen.qml`
- `src/views/screens/WelcomeScreen.qml`
- `src/views/screens/WizardScreen.qml`
- `src/views/screens/ScanScreen.qml`
- `src/views/screens/HomeScreen.qml`
- `src/views/components/AnimatedZowi.qml`

## Unchanged (compatible API)

- `QBluetoothSocket::connectToService(QBluetoothAddress, QBluetoothUuid)` — same signature
- `QBluetoothDeviceDiscoveryAgent::start(ClassicMethod)` — same
- `QQmlApplicationEngine`, `QQmlContext`, `Q_PROPERTY`, `Q_INVOKABLE` — same
- `QTimer` — same
- CMake `CMAKE_AUTOMOC` — still works
- `zowi_core` — no changes needed (pure C++)
- CLI `session`, `config`, `translate` commands — no changes needed

## Summary of changes

| File | Change |
|------|--------|
| `CMakeLists.txt` (root) | `Qt6` → `Qt5`, C++20 → C++17 |
| `src/backends/bt_qt/CMakeLists.txt` | `Qt6::*` → `Qt5::*` |
| `src/gui/CMakeLists.txt` | `qt6_` → `qt5_`, `Qt6::*` → `Qt5::*` |
| `src/cli/CMakeLists.txt` | `Qt6::Bluetooth` → `Qt5::Bluetooth` |
| `src/backends/bt_qt/qt_bluetooth_backend.cpp` | 2 lines: `errorOccurred` → `error` with QOverload |
| `src/views/**/*.qml` | 7 files: import versions `6.0` → `2.15` |

**Total: ~12 changes across ~11 files.**
