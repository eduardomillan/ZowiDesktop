# Migration: MVVM → API + CLI + Frontend

## Why we migrated

The original architecture was a **monolithic MVVM** pattern tightly coupled to Qt:

- Business logic lived in Qt `QObject` subclasses (`SessionService`, `BluetoothService`, `TranslationEngine`)
- Every service required Qt6::Core and Qt6::Bluetooth
- Testing required the full Qt stack (QTest, QML context)
- No way to test or debug Bluetooth/translation without launching the GUI
- The CLI did not exist — no terminal-based debugging

### Problems

| Problem | Impact |
|---------|--------|
| Core logic tied to Qt | Can't run tests without Qt installed |
| No CLI | Must launch GUI to test BT scanning, session read/write |
| Services depend on QObject | Heavy compile times, complex test setup |
| Translation engine tied to Qt resources | Can't use it independently |
| Bluetooth logic coupled to QML | Hard to swap backends (e.g., native Win32 BT) |

## What changed

### Before (MVVM)

```
src/
├── main.cpp              # Single entry point
├── services/             # Business logic (Qt-coupled)
│   ├── BluetoothService
│   ├── SessionService
│   └── TranslationEngine
├── controllers/          # ViewModel layer
│   ├── BluetoothController
│   ├── SessionController
│   ├── TranslatorController
│   └── ConfigController
└── views/                # QML screens
```

### After (API + CLI + Frontend)

```
src/
├── core/                 # Pure C++ library (no Qt)
│   ├── include/zowi/     # Public API headers
│   ├── src/              # Implementations
│   └── tests/            # Unit tests (no Qt)
├── backends/bt_qt/       # Qt Bluetooth backend (pluggable)
├── gui/                  # Qt 6 / QML application
│   ├── controllers/      # Qt adapters wrapping core
│   └── main.cpp
├── cli/                  # CLI tool
│   └── main.cpp
└── views/                # QML screens (unchanged)
```

### What was deleted

| File | Reason |
|------|--------|
| `src/services/*` | Replaced by `src/core/` (pure C++) |
| `src/controllers/*` (old) | Replaced by `src/gui/controllers/` (Qt adapters) |
| `src/main.cpp` (old) | Replaced by `src/gui/main.cpp` |
| `src/BtTest.cpp` | Replaced by `zowi_cli scan` |
| `src/tests/*` (old) | Replaced by `src/core/tests/` (no Qt dependency) |
| `zowi_services_legacy` target | Removed |
| `zowi_controllers_legacy` target | Removed |
| `zowi_core_legacy` target | Removed |

### What was created

| File | Purpose |
|------|---------|
| `src/core/` | Pure C++ library: SessionStore, ConfigStore, TranslationEngine, BluetoothApi |
| `src/core/tests/` | 3 unit tests running without Qt |
| `src/backends/bt_qt/` | Qt Bluetooth implementation of abstract BluetoothApi |
| `src/gui/controllers/` | Qt adapter classes wrapping core for QML exposure |
| `src/gui/main.cpp` | New entry point with hot-reload, context wiring |
| `src/cli/main.cpp` | CLI tool with session, config, translate, scan subcommands |
| `build_cli.sh` | Helper script to build and demo CLI |

## Benefits

### 1. Testable without Qt

Core library tests compile and run with zero Qt dependency:

```bash
cmake -B build -DZOWI_BUILD_CLI=OFF -DZOWI_BUILD_GUI=OFF
cmake --build build
ctest --test-dir build
# 3/3 tests pass in <0.01s
```

### 2. CLI for debugging

Terminal-based access to all core functionality:

```bash
./zowi_cli session list          # Inspect persisted state
./zowi_cli config get know_more  # Read config values
./zowi_cli translate -s "Hola"   # Test translations
./zowi_cli scan -t 5             # Scan BT devices without GUI
```

### 3. Pluggable backends

Bluetooth backend is abstract (`BluetoothApi`). The Qt implementation (`zowi_bt_qt`) is one option. A native Win32 backend or a mock for testing can be swapped without touching core code.

### 4. Faster iteration

- Core changes: recompile only `zowi_core` (no Qt recompilation)
- CLI changes: recompile only `zowi_cli` (no QML reload)
- QML changes: hot-reload without C++ recompilation

### 5. Independent consumers

CLI and GUI are separate executables. You can:
- Build CLI without GUI (`-DZOWI_BUILD_GUI=OFF`)
- Build GUI without CLI (`-DZOWI_BUILD_CLI=OFF`)
- Build both (default)

## Migration checklist

- [x] Create `src/core/` with pure C++ classes
- [x] Add unit tests for core (no Qt)
- [x] Create `src/backends/bt_qt/` implementing BluetoothApi
- [x] Create `src/gui/` with new entry point and Qt adapters
- [x] Create `src/cli/` with CLI11 subcommands
- [x] Remove old `src/services/`, `src/controllers/`, `src/tests/`
- [x] Remove legacy CMake targets
- [x] Add `build_cli.sh` helper
- [x] Update ARCHITECTURE.md
- [x] This document (MIGRATION.md)
