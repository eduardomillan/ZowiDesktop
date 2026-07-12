# bt_native — Windows Bluetooth DLL

Native Windows Bluetooth library for ZowiDesktop. Provides SPP (RFCOMM) Bluetooth
communication on Windows when cross-compiled Qt6 Bluetooth is unavailable.

## Prerequisites

- **Windows 10+**
- **Visual Studio 2019+** (Community edition is fine) with "Desktop development with C++" workload
- **CMake 3.16+** (included with Visual Studio or install separately)

## Build Steps

### 1. Open Developer Command Prompt

Search for "Developer Command Prompt for VS 2022" in the Start menu.
This sets up the MSVC compiler environment.

### 2. Navigate to bt_native

```cmd
cd path\to\ZowiDesktop\packaging\windows\bt_native
```

### 3. Build

```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 4. Copy the DLL

The built DLL will be at:

```
build\bin\Release\bt_native.dll
```

Copy it next to `ZowiDesktop.exe` in the portable package:

```
ZowiDesktop-windows-x86_64-build-YYYYMMDD\
├── ZowiDesktop.exe
├── bt_native.dll        <-- HERE
├── Qt6Core.dll
├── ...
```

## Integration with ZowiDesktop

The main app automatically detects `bt_native.dll` at startup:

1. If found → uses native Windows Bluetooth (WinRT API)
2. If not found → falls back to Qt Bluetooth (works on Linux only)

No code changes needed in the main project. Just place the DLL next to the exe.

## API Reference

See `bt_native.h` for the full C API. Key functions:

| Function | Description |
|----------|-------------|
| `bt_init()` | Initialize Bluetooth adapter |
| `bt_start_discovery()` | Scan for nearby devices |
| `bt_connect(address)` | Connect to device by MAC address |
| `bt_send(data, length)` | Send data over SPP |
| `bt_disconnect()` | Disconnect from device |
| `bt_cleanup()` | Release all resources |

## Implementing Real Bluetooth

The current `bt_native.cpp` is a **stub** (no-op). To implement real Bluetooth:

1. Use **C++/WinRT** for the Windows.Devices.Bluetooth API
2. Implement device discovery via `BluetoothAdapter::GetDefaultAsync()`
3. Implement SPP connection via `RfcommDeviceServicesResult`
4. See: https://learn.microsoft.com/en-us/windows/uwp/devices-sensors/bluetooth

## Troubleshooting

- **"bt_native.dll not found"**: Place the DLL next to ZowiDesktop.exe
- **"Could not load bt_native.dll"**: Ensure it was compiled for x64 with the same runtime
- **Build fails**: Make sure you're using the Developer Command Prompt (not regular cmd)
