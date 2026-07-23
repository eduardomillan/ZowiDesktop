# Building Zowi Desktop

## Prerequisites

- CMake 3.16+
- Qt 5.15+ or Qt 6.2+ (Core, Quick, QuickControls2, Bluetooth)
- C++17 compiler for Qt 5, C++20 for Qt 6 (g++ or clang++)

```bash
# Lliurex 23 / Ubuntu Jammy
sudo apt install cmake g++ qt6-base-dev qt6-declarative-dev qt6-connectivity-dev \
    libbluetooth-dev libgl1-mesa-dev libxkbcommon-dev nlohmann-json3-dev libcli11-dev
```

## Quick start (native Linux)

```bash
./build.sh               # build everything (GUI + CLI) with Qt 6
./build/src/gui/ZowiDesktop      # run the GUI
```

For **CLI only**:

```bash
./build.sh --cli
./build/src/cli/zowi_cli --help
```

For **GUI only** (no CLI):

```bash
./build.sh --gui
./build/src/gui/ZowiDesktop
```

To build with **Qt 5** instead of Qt 6:

```bash
./build.sh -5            # everything with Qt 5
./build.sh -5 --cli      # CLI only with Qt 5
```

## Manual CMake invocations

```bash
# Everything (GUI + CLI) with Qt 6
cmake -B build -DZOWI_BUILD_GUI=ON -DZOWI_BUILD_CLI=ON
cmake --build build

# GUI only
cmake -B build -DZOWI_BUILD_GUI=ON -DZOWI_BUILD_CLI=OFF
cmake --build build

# CLI only
cmake -B build -DZOWI_BUILD_GUI=OFF -DZOWI_BUILD_CLI=ON
cmake --build build --target zowi_cli

# With Qt 5
cmake -B build -DZOWI_QT_VERSION=5 -DZOWI_BUILD_GUI=ON -DZOWI_BUILD_CLI=ON
cmake --build build

# Disable tests
cmake -B build -DBUILD_TESTS=OFF
cmake --build build
```

## Build targets

| Target | Type | CMake option |
|--------|------|-------------|
| `ZowiDesktop` | Executable (Qt GUI) | `ZOWI_BUILD_GUI=ON` |
| `zowi_screen_preview` | Executable (screen preview tool) | `ZOWI_BUILD_GUI=ON` |
| `zowi_cli` | Executable (terminal) | `ZOWI_BUILD_CLI=ON` |
| `zowi_core` | Static library | Always built |
| `zowi_firmware` | Static library (STK500v1/Optiboot) | Always built |
| `zowi_bt_qt` | Static library (Qt Bluetooth SPP) | `ZOWI_BUILD_GUI=ON` or `ZOWI_BUILD_CLI=ON` |
| `zowi_bt_serial` | Static library (RFCOMM TTY + USB serial, POSIX only) | `ZOWI_BUILD_GUI=ON` or `ZOWI_BUILD_CLI=ON` (excluded on Windows) |
| `test_*` | Test executables | `BUILD_TESTS=ON` |

## Platform builds

| Platform | Output | How to build |
|---|---|---|
| **Linux (native)** | `build/src/gui/ZowiDesktop` | `./build.sh` |
| **Linux (AppImage)** | `build/ZowiDesktop-<version>-x86_64.AppImage` | `./packaging/linux/create-appimage.sh` |
| **Linux (.deb)** | `build/zowi-desktop_<version>-1+<distro>_amd64.deb` | `./packaging/linux/create-deb.sh` |
| **Windows (.zip)** | `build-windows/ZowiDesktop-windows-x86_64.zip` | `./packaging/windows/create-portable-zip.sh` |

## Linux AppImage

The script downloads `linuxdeploy` and its Qt plugin automatically, then
bundles the application and all dependencies into a portable AppImage.

No special privileges required. The resulting file can be run on any
Linux distribution without installing Qt.

```bash
./packaging/linux/create-appimage.sh
```

The version is read from `CMakeLists.txt`. Resources are compiled without
zstd compression (`--no-zstd`) to ensure compatibility with older Qt versions.

## Linux Debian packages

The script builds `.deb` packages for Ubuntu/Lliurex distributions. The version
is read from `CMakeLists.txt` and release notes are extracted from `CHANGELOG.md`.

```bash
# Build for Ubuntu 22.04 (jammy)
DISTRO_SUFFIX=jammy ./packaging/linux/create-deb.sh

# Build for Ubuntu 24.04 (noble)
DISTRO_SUFFIX=noble ./packaging/linux/create-deb.sh
```

The resulting `.deb` files are placed in `build/`.

## GitHub Releases

To create a GitHub release with the AppImage and Debian packages:

```bash
./packaging/create-gh-release.sh
```

The script:
- Reads the version from `CMakeLists.txt`
- Verifies all required artifacts exist (AppImage + .deb jammy + .deb noble)
- Extracts changelog entries from `debian/changelog`
- Creates a git tag `v<version>` and pushes it
- Creates a GitHub Release with the artifacts attached

Requires the `gh` CLI authenticated (`gh auth login`).

## Windows portable .zip

Cross-compilation requires:

1. **MinGW toolchain**
   ```bash
   sudo apt install mingw-w64 mingw-w64-tools
   ```

2. **Qt 6 for MinGW** — download from https://www.qt.io/download-open-source
   and install selecting *Qt 6.x → MinGW 64-bit*.

3. Set `QT_MINGW_PATH` and run the script:
   ```bash
   export QT_MINGW_PATH=~/Qt/6.5.2/mingw_64
   ./packaging/windows/create-portable-zip.sh
   ```

The script runs `windeployqt` to collect all required DLLs and packages
everything into a portable `.zip`. No installation needed on the target
Windows machine.

> **Note:** Cross-compiled builds from Linux do not include working Bluetooth
> (Qt6 Bluetooth requires WinRT, only available with MSVC).
> See [bt_native](packaging/windows/bt_native/README.md) for a native Windows
> Bluetooth DLL that can be compiled separately on Windows with Visual Studio.

## Windows native Bluetooth (bt_native)

For full Bluetooth support on Windows, a native DLL can be compiled on a Windows
machine with Visual Studio and placed next to `ZowiDesktop.exe`. The app
auto-detects it at startup.

See: [packaging/windows/bt_native/README.md](packaging/windows/bt_native/README.md)
