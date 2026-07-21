# Windows Port — Porting ZowiDesktop to Windows

## Table of Contents

- [Overview](#overview)
- [Effort Summary](#effort-summary)
- [Key Problems](#key-problems)
  - [1. bt_serial — POSIX Serial Backend](#1-bt_serial--posix-serial-backend)
  - [2. bt_qt — Qt Bluetooth with BlueZ Agent](#2-bt_qt--qt-bluetooth-with-bluez-agent)
  - [3. CLI rfcomm Commands](#3-cli-rfcomm-commands)
  - [4. bt_native.dll — Stub Only](#4-bt_nativedll--stub-only)
- [Already Cross-Platform](#already-cross-platform)
  - [Build System](#build-system)
  - [Session Store](#session-store)
- [Phased Approach](#phased-approach)
  - [Phase 1 (High Effort): Windows Bluetooth](#phase-1-high-effort-windows-bluetooth)
  - [Phase 2 (High Effort): Windows Serial](#phase-2-high-effort-windows-serial)
  - [Phase 3 (Medium Effort): CLI Windows Adaptation](#phase-3-medium-effort-cli-windows-adaptation)
  - [Phase 4 (Low Effort): Build System Polish](#phase-4-low-effort-build-system-polish)
- [Faster Alternative: USB-Only Mode](#faster-alternative-usb-only-mode)
- [Dependencies](#dependencies)
- [Build on Windows](#build-on-windows)
- [Testing](#testing)
- [See Also](#see-also)

## Overview

Porting ZowiDesktop to Windows is feasible but requires significant work on the Bluetooth and serial backends, which are currently POSIX/Linux-specific. Several components (core, build system, session store) are already cross-platform or have Windows scaffolding in place.

## Effort Summary

| Component | Effort | Current Status |
|-----------|--------|----------------|
| **Core library** | Low | Already works on Windows |
| **Serial backend** | High | 100% POSIX - needs Win32 Serial API rewrite |
| **Qt Bluetooth (bt_qt)** | High | Uses D-Bus/BlueZ Agent - no Windows equivalent |
| **CLI rfcomm commands** | Medium | Linux commands - not applicable on Windows |
| **Terminal input (raw mode)** | Medium | Uses termios.h - needs Windows Console API |
| **Build system** | Low | Already has WIN32 conditionals |
| **Session store** | Done | Already handles Windows paths |

## Key Problems

### 1. bt_serial — POSIX Serial Backend

The entire backend uses POSIX APIs:

| POSIX API | Purpose | Windows Alternative |
|-----------|--------|---------------------|
| `<termios.h>` | Serial port configuration | Win32 Serial API |
| `<unistd.h>` | read(), write(), close() | ReadFile, WriteFile, CloseHandle |
| `<dirent.h>` | Directory listing for /dev/tty* | QueryDosDevice, SetupDi |
| `<sys/ioctl.h>` | DTR pulse for reset | EscapeCommFunction |
| `O_NOCTTY`, `O_NONBLOCK` | Open flags | FILE_FLAG_* |
| `B9600`, `B115200` | Baud rate constants | CBR_* constants |

Key functions needing rewrite:
- `baudToSpeed()` — POSIX speed_t constants
- `SerialBluetoothBackend::connect()` — full termios setup
- `SerialBluetoothBackend::pulseReset()` — uses ioctl with TIOCM_DTR
- `SerialBluetoothBackend::listSerialPorts()` — scans /dev/ttyUSB* and /dev/ttyACM*

### 2. bt_qt — Qt Bluetooth with BlueZ Agent

The Qt Bluetooth backend works on Windows via WinRT, but the BlueZ Agent integration is Linux-only:

- `BlueZAgent` class handles automatic PIN entry ("1234") for HC-05 pairing
- Uses D-Bus for agent registration
- No Windows equivalent mechanism

Windows would need either:
- A custom Windows Runtime implementation
- Or user manual pairing before connecting

### 3. CLI rfcomm Commands

The CLI explicitly calls `rfcomm bind` and `rfcomm release` which are Linux/BlueZ tools:

```cpp
std::system("rfcomm bind 0 " + address + " 1");
std::system("rfcomm release 0");
```

These need conditional compilation or removal on Windows.

### 4. bt_native.dll — Stub Only

A stub exists at `packaging/windows/bt_native/` but:
- No actual WinRT implementation
- Not integrated into the build system
- Not loaded at runtime

## Already Cross-Platform

### Build System

CMakeLists.txt already has WIN32 conditionals:

```cmake
if((ZOWI_BUILD_CLI OR ZOWI_BUILD_GUI) AND NOT WIN32)
    add_subdirectory(bt_serial)
endif()
```

### Session Store

`session_store.cpp` already handles Windows paths:

```cpp
#ifdef _WIN32
    path = qgetenv("APPDATA") + "/" + org + "/" + app;
#else
    path = QDir::homePath() + "/.config/" + org + "/" + app;
#endif
```

## Phased Approach

### Phase 1 (High Effort): Windows Bluetooth

Options:
1. Complete `bt_native.dll` with real WinRT code
2. Modify `bt_qt` to use Windows WinRT APIs via Qt's WinRT backend

Recommended: Start with USB-only mode (Phase 2) to get something working, then add Bluetooth.

### Phase 2 (High Effort): Windows Serial

Create `win32_serial_backend.cpp` with Win32 Serial API:

```cpp
// Pseudocode structure
class Win32SerialBackend : public BluetoothBackend {
    HANDLE m_handle;
    
    bool connect(const std::string& port) {
        m_handle = CreateFileA(port.c_str(), 
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, NULL);
        // SetCommState, SetCommTimeouts
    }
    
    void disconnect() { CloseHandle(m_handle); }
    
    std::vector<std::string> listPorts() {
        // Use QueryDosDevice or SetupDi API
    }
};
```

Replace `/dev/tty*` path enumeration with Windows COM ports (COM1, COM2, ...).

### Phase 3 (Medium Effort): CLI Windows Adaptation

1. Make `rfcomm bind/release` no-ops on Windows
2. Replace raw terminal mode with Windows Console API:
   - `SetConsoleMode()` for raw input
   - `ReadConsoleInput()` for key detection

### Phase 4 (Low Effort): Build System Polish

1. Ensure Visual Studio project generation works
2. Test with native Windows Qt build (not MinGW cross-compile)
3. Verify all WIN32 conditionals work correctly

## Faster Alternative: USB-Only Mode

If Windows Bluetooth is deferred, USB-only mode is much simpler:

1. Rewrite `bt_serial` for Windows
2. GUI works with direct USB connection
3. CLI works with `--backend usb --tty COM3`

This gives a working Windows product faster, with Bluetooth as a future enhancement.

## Dependencies

| Dependency | Windows Alternative | Notes |
|------------|-------------------|-------|
| BlueZ/D-Bus | Windows WinRT Bluetooth | Required for bt_qt |
| rfcomm | N/A | Not applicable |
| /dev/ttyUSB* | COM1, COM2, ... | Serial backend |
| termios/ioctl | Win32 Serial API | Serial backend |
| cap_net_admin | Run as administrator | Build only |
| setcap | N/A | Run as administrator |

## Build on Windows

Existing Windows build scripts:
- `packaging/windows/build-portable.bat` — MinGW build
- `packaging/windows/create-portable-zip.sh` — Cross-compile with MinGW

Qt6 Bluetooth on Windows requires:
- Windows 10 SDK
- WinRT Bluetooth APIs
- `Qt6Bluetooth` linking (already in CMakeLists.txt)

## Testing

1. Test with USB cable connection first (simpler)
2. Use a USB-UART adapter (FTDI, CH340, etc.)
3. Verify serial port enumeration works
4. Then attempt Bluetooth pairing

## See Also

- [ZOWI_CLI_HOWTO.md](../ZOWI_CLI_HOWTO.md) — CLI usage
- [BUILD.md](../BUILD.md) — Build system overview
- [ARCHITECTURE.md](../ARCHITECTURE.md) — Project architecture
