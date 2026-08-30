# Windows Support — ZowiDesktop on Windows

## Table of Contents

- [Overview](#overview)
- [What Was Added (Windows Layer)](#what-was-added-windows-layer)
- [Architecture: Backend Polymorphism](#architecture-backend-polymorphism)
- [CMake Build Selection](#cmake-build-selection)
- [Runtime Backend Selection](#runtime-backend-selection)
- [Impact on Main/Linux Development](#impact-on-mainlinux-development)
- [Build on Windows](#build-on-windows)
- [Testing](#testing)
- [See Also](#see-also)

---

## Overview

Windows support is **fully implemented** in the `windows` branch (9 commits ahead of `main`). The implementation adds two native Windows backends — Bluetooth (WinRT) and Serial/USB (Win32 API) — while keeping all business logic, UI, and core libraries **shared and unchanged** with Linux/macOS.

**Status:** Ready for fast-forward merge into `main`.

---

## What Was Added (Windows Layer)

| Component | Files | Lines | Description |
|-----------|-------|-------|-------------|
| **Native Bluetooth Backend (WinRT)** | `src/backends/bt_native/native_bluetooth_backend.{h,cpp}` | ~970 | Implements `BluetoothApi` using `winrt::Windows::Devices::Enumeration::DeviceWatcher` + `RfcommDeviceService`. Handles automatic pairing (PIN 1234 for HC-05), auto-reconnect, real-time discovery. |
| **Native Serial/USB Backend (Win32)** | `src/backends/bt_serial_win/win_serial_backend.{h,cpp}` | ~320 | Implements `BluetoothApi` over Win32 `CreateFile/ReadFile/WriteFile` on COM ports. Controls DTR (avoids accidental reset), configurable baud/boot delay, COM port enumeration. |
| **Build System Integration** | `CMakeLists.txt`, `build.bat` | ~220 | Detects Windows SDK, enables C++/WinRT (`winrt` NuGet/vcpkg), adds `WIN32` guard for backends, runs `windeployqt` automatically. |
| **GUI/CLI Windows Fixes** | `RobotController.cpp`, `cli_util.cpp` | ~360 | DTR handling, port probing without reset, raw console mode (CLI interactive), fix crash on "Forget Zowi". |
| **Packaging** | `packaging/windows/` | ~175 | Inno Setup installer (`.iss`), portable/build scripts. |
| **CI/CD / Release** | `packaging/create-gh-release.sh`, `packaging/windows/installer/*` | ~80 | Manual release (no GitHub Actions workflow): builds AppImage + .deb (jammy/noble) + Windows zip/installer, creates the GitHub Release, and publishes the signed apt repo on `gh-pages` (coexists with website). |
| **Core/UI Minor** | `translation_engine.cpp`, QML screens | ~60 | i18n scope fixes, DevOverlay visibility, `.ico` icon, app manifest. |

**Total: ~1,700 net lines added across 41 files.**

---

## Architecture: Backend Polymorphism

All backends implement the same abstract interface:

```
src/core/include/zowi/bluetooth_api.h  →  BluetoothApi (pure virtual)
```

```
src/backends/
├── bt_qt/              # Linux/macOS: Qt Bluetooth (QBluetoothSocket + BlueZ D-Bus Agent)
├── bt_native/          # Windows:    C++/WinRT (Windows.Devices.Bluetooth + SPP)
├── bt_serial/          # Linux/macOS: Serial RFCOMM (/dev/rfcommX, termios)
└── bt_serial_win/      # Windows:    Serial COM (CreateFile/ReadFile/WriteFile, Win32 API)
```

**No business logic duplication** — `RobotController`, `SessionStore`, `ConfigStore`, firmware upload, protocol parsing, CLI commands all live in shared code and call `BluetoothApi` polymorphically.

---

## CMake Build Selection

`CMakeLists.txt` (lines 67–96):

```cmake
# Linux/macOS
if((ZOWI_BUILD_GUI OR ZOWI_BUILD_CLI) AND NOT WIN32)
    add_subdirectory(src/backends/bt_qt)       # Bluetooth
    add_subdirectory(src/backends/bt_serial)   # USB/Serial
endif()

# Windows
if((ZOWI_BUILD_GUI OR ZOWI_BUILD_CLI) AND WIN32)
    add_subdirectory(src/backends/bt_native)       # Bluetooth nativo (WinRT)
    add_subdirectory(src/backends/bt_serial_win)   # USB/Serial COM
endif()
```

- **Linux builds never compile Windows code** (no WinRT, no Win32 API).
- **Windows builds never compile Linux backends** (no BlueZ, no `termios`).
- Core (`zowi_core`), firmware (`zowi_firmware`), CLI logic, GUI, tests — **identical** on both platforms.

---

## Runtime Backend Selection

`src/gui/controllers/RobotController.cpp` (lines 118–145):

```cpp
void RobotController::useBluetoothBackend()
{
    if (m_backend && m_backendKind == Bluetooth) return;
#ifdef ZOWI_HAVE_NATIVE_BT
    m_backend = std::make_unique<zowi::NativeBluetoothBackend>();  // Windows
#else
    m_backend = std::make_unique<zowi::QtBluetoothBackend>();      // Linux/macOS
#endif
    m_backendKind = Bluetooth;
    wireBackend();
    setActiveTransport(Bluetooth);
}

void RobotController::useSerialBackend()
{
#ifdef ZOWI_HAVE_SERIAL
    if (m_backend && m_backendKind == Usb) return;
    auto serial = std::make_unique<SerialBackend>();
    serial->setBaudRate(m_usbBaud);
    serial->setBootDelayMs(5000);
    m_backend = std::move(serial);
    m_backendKind = Usb;
    wireBackend();
    setActiveTransport(Usb);
#else
    useBluetoothBackend();
#endif
}
```

CLI uses the same pattern in `src/cli/cli_util.cpp` (lines 21–28, 374–378).

---

## Impact on Main/Linux Development

| Area | Impact |
|------|--------|
| **Core library** (`src/core/`) | **None** — unchanged. Tests run identically (`ctest --test-dir build`). |
| **Firmware upload** (`src/firmware/`) | **None** — pure C++, STK500v1/Optiboot logic shared. |
| **Bluetooth backend Linux** (`bt_qt/`) | **Unaffected** — still uses Qt Bluetooth + BlueZ D-Bus agent (PIN 1234). |
| **Serial backend Linux** (`bt_serial/`) | **Unaffected** — still uses `termios` + `rfcomm bind` (needs `CAP_NET_ADMIN`). |
| **RobotController** (state machine, transport selection, firmware restore) | **Unified** — same `Situation` enum, same auto-detection, same `switchTransport()`. Only `#ifdef` chooses backend concrete type. |
| **CLI commands** (`connect`, `flash`, `control`, `monitor`) | **Unified** — same argument parsing, same flows. `#ifdef _WIN32` only for raw console mode + backend instantiation. |
| **Build system (Linux)** | **No changes** — `./build.sh` (or `cmake -DZOWI_BUILD_GUI=ON -DZOWI_BUILD_CLI=ON`) ignores Windows subdirs automatically. |
| **Packaging (Linux)** | **Unaffected** — `.deb`, AppImage, apt repo built and published manually via `packaging/create-gh-release.sh` (no Linux CI job). |
| **Translations / QML / Resources** | **Shared** — same `.ts/.json`, same `TranslationEngine`, same QML files. |

**Bottom line:** Adding Windows support added **zero lines to Linux-specific code**. The "Windows layer" is strictly additive backends + compile guards.

---

## Build on Windows

**Prerequisites:**
- Visual Studio 2022 (x64 Native Tools Command Prompt)
- Qt 6 (e.g., `C:\Qt\6.5.2\msvc2019_64`)
- CMake ≥ 3.16

**Build:**
```cmd
# From x64 Native Tools Command Prompt for VS 2022
build.bat              # GUI + CLI (Release)
build.bat --gui        # GUI only
build.bat --cli        # CLI only
```

The script:
1. Runs CMake with `-G "Visual Studio 17 2022" -A x64`
2. Builds with `cmake --build build --config Release`
3. Runs `windeployqt --qmldir src\views` automatically after GUI build
4. Output: `build/src/gui/Release/ZowiDesktop.exe` + Qt DLLs, `build/src/cli/Release/zowi_cli.exe`

**CLI Bluetooth capability (optional):**
```cmd
sudo setcap cap_net_admin+ep build/src/cli/zowi_cli.exe
# Re-apply after every rebuild. Script: scripts/grant_bluetooth_cap.sh
```

---

## Testing

1. **USB/Serial first** (simpler): Connect Zowi via USB, run `zowi_cli.exe --backend usb --tty COM3 connect`
2. **Bluetooth**: Pair Zowi in Windows Settings (PIN 1234), then `zowi_cli.exe connect`
3. **GUI**: Run `ZowiDesktop.exe` — auto-detects USB/Bluetooth, shows situation-aware UI
4. **Firmware flash**: `zowi_cli.exe flash --firmware path/to/firmware.hex`
5. **Unit tests**: `ctest --test-dir build --output-on-failure` (core tests only, no Qt)

---

## See Also

- [ZOWI_CLI_HOWTO.md](../ZOWI_CLI_HOWTO.md) — CLI usage (cross-platform)
- [BUILD.md](../BUILD.md) — Build system overview
- [ARCHITECTURE.md](../ARCHITECTURE.md) — Project architecture (backend polymorphism)
- `.local/transport_thoughts.md` — Transport state machine design notes