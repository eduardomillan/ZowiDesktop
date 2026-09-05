# Building Zowi Desktop

## Table of contents

- [Prerequisites](#prerequisites)
- [Quick start (native Linux)](#quick-start-native-linux)
- [Running](#running)
  - [Linux](#linux)
  - [Windows](#windows)
  - [Environment variables](#environment-variables)
- [Manual CMake invocations](#manual-cmake-invocations)
- [Build targets](#build-targets)
- [Platform builds](#platform-builds)
- [Linux AppImage](#linux-appimage)
- [Linux Debian packages](#linux-debian-packages)
- [GitHub Releases](#github-releases)
- [Windows builds](#windows-builds)
  - [On a Windows machine (MSVC)](#on-a-windows-machine-msvc)
  - [Windows installer (.exe, Inno Setup)](#windows-installer-exe-inno-setup)
  - [Windows portable .zip (MSVC)](#windows-portable-zip-msvc)
- [Windows CI (GitHub Actions + MSVC)](#windows-ci-github-actions--msvc)
- [Windows native Bluetooth (bt_native)](#windows-native-bluetooth-bt_native)

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

## Running

### Linux

**GUI (ZowiDesktop)**:

```bash
./build.sh --gui
./build/src/gui/ZowiDesktop
```

**CLI (zowi_cli)**:

```bash
./build.sh --cli
./build/src/cli/zowi_cli --help
./build/src/cli/zowi_cli calibrate        # interactive servo calibration
./build/src/cli/zowi_cli scan              # scan for nearby Zowis
```

> **Bluetooth permissions**: the CLI needs `cap_net_admin` to open Bluetooth
> sockets without root. After every rebuild:
> ```bash
> sudo setcap cap_net_admin+ep build/src/cli/zowi_cli
> ```
> A helper script is provided: `sudo ./scripts/grant_bluetooth_cap.sh`

**Both (GUI + CLI)**:

```bash
./build.sh
./build/src/gui/ZowiDesktop               # launch the GUI
./build/src/cli/zowi_cli --help            # or use the CLI
```

### Windows

Open a **x64 Native Tools Command Prompt for VS 2022** (search "x64" in the
Start menu). This sets up the MSVC compiler and Windows SDK environment.

**GUI (ZowiDesktop)**:

```bat
build.bat --gui
build\src\gui\Release\ZowiDesktop.exe
```

**CLI (zowi_cli)**:

```bat
build.bat --cli
build\src\cli\Release\zowi_cli.exe --help
build\src\cli\Release\zowi_cli.exe calibrate
build\src\cli\Release\zowi_cli.exe scan
```

**Both (GUI + CLI)**:

```bat
build.bat
build\src\gui\Release\ZowiDesktop.exe
build\src\cli\Release\zowi_cli.exe --help
```

`windeployqt` runs automatically after the GUI build to copy Qt DLLs and QML
files alongside the executable.

> **Tip**: `build.bat --demo` compiles the CLI and runs a quick demo of the
> most common commands (session, config, translate, scan).

### Environment variables

The following environment variables can be set at runtime to influence
application behavior for testing:

| Variable | Accepted values | Effect |
|---|---|---|
| `DEV_MODE` | `1`, `true`, `on` (case-insensitive) | Enables dev mode and shows the dev overlay on the GUI. Overrides the `dev_mode` value in `src/config.json`. |

**Examples:**

Linux:
```bash
DEV_MODE=1 ./build/src/gui/ZowiDesktop          # dev mode on
DEV_MODE=true ./build/src/gui/ZowiDesktop       # same
```

Windows (cmd):
```bat
set DEV_MODE=1 && build\src\gui\Release\ZowiDesktop.exe
```

Windows (PowerShell):
```powershell
$env:DEV_MODE="1"; build\src\gui\Release\ZowiDesktop.exe
```

Additionally, the **screen preview tool** (`zowi_screen_preview`) accepts:

| Variable | Effect |
|---|---|
| `PREVIEW_GRAB_DIR` | When set, captures a headless screenshot of the preview window to `<dir>/<ScreenName>_step<N>.png` and exits. Useful for iterating on QML layouts without a display. |

```bash
PREVIEW_GRAB_DIR=/tmp/grabs build/src/gui/zowi_screen_preview src/views/screens/SettingsScreen.qml
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
| **Linux (AppImage)** | `dist/ZowiDesktop-<version>-x86_64.AppImage` | `./packaging/linux/create-appimage.sh` |
| **Linux (.deb)** | `dist/zowi-desktop_<version>-1+<distro>_amd64.deb` | `./packaging/linux/create-deb.sh` |
| **Windows (zip + installer)** | `dist/` | Windows machine (`build.bat`, `build-installer.bat`) or GitHub Actions (`windows.yml`) |

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

The resulting `.deb` files are placed in `dist/`.

## GitHub Releases

Releases are created **manually** (no CI workflow). To create a GitHub release
attached with the AppImage and Debian packages:

> The complete end-to-end release guide (version bumps, all platform artifacts,
> apt repo publishing) lives in [docs/project/RELEASE.md](RELEASE.md).

```bash
# GitHub Release with Linux artifacts (+ Windows zip/installer if present)
./packaging/create-gh-release.sh

# Same, and also publish the signed apt repo (jammy+noble) to gh-pages
./packaging/create-gh-release.sh --with-apt
```

The script:
- Reads the version from `CMakeLists.txt`
- Verifies the required artifacts exist (AppImage + .deb jammy + .deb noble)
- Attaches the Windows portable zip and installer too, if found in
  `dist/`
- Extracts changelog entries from `debian/changelog`
- Creates a git tag `v<version>` and pushes it
- Creates a GitHub Release with the artifacts attached
- With `--with-apt`, signs and publishes the apt repo (jammy+noble) under
  `docs/` on `gh-pages` via `packaging/publish-apt-repo.sh` (keeps the website)

Requires the `gh` CLI authenticated (`gh auth login`). Publishing the apt repo
additionally requires `aptly`, `gnupg`, the repo's GPG signing key on this
machine, and `GPG_PASSPHRASE` (or `APTLY_GPG_PASSPHRASE`) set.

## Windows builds

Windows requires the **MSVC toolchain + Windows SDK** (the native WinRT
`bt_native` backend and the Win32 `bt_serial_win` backend do not compile with
MinGW). There is no cross-compile from Linux — build on a real Windows machine
or use GitHub Actions.

### On a Windows machine (MSVC)

From a **x64 Native Tools Command Prompt for VS 2022**:

```bat
build.bat --gui          :: GUI only
build.bat --cli          :: CLI only
build.bat                :: GUI + CLI
```

`windeployqt --qmldir src\views` runs automatically to bundle Qt DLLs/QML.

### Windows installer (.exe, Inno Setup)

From the same MSVC prompt, with Inno Setup 6 installed:

```bat
packaging\windows\installer\build-installer.bat
```

Produces `dist\ZowiDesktop-<version>-setup-x64.exe` from
everything in `dist\`.

### Windows portable .zip (MSVC)

`packaging/windows/build-portable.bat` builds `ZowiDesktop.exe` and
`zowi_cli.exe` and packs them (with Qt DLLs/QML via `windeployqt`) into
`dist\ZowiDesktop-<version>-windows-x86_64.zip`.

> The old `create-portable-zip.sh` MinGW cross-compile script has been removed:
> the WinRT `bt_native` backend requires the Windows SDK and does not build with
> MinGW, so it never produced a functional Bluetooth build.

## Windows CI (GitHub Actions + MSVC)

The repository ships `.github/workflows/windows.yml` which compiles Windows
artifacts on a `windows-latest` runner using the MSVC 2022 toolchain and Qt 6.8:

- **Trigger**: manual (`workflow_dispatch`) — consistent with the manual-
  release philosophy (no automatic releases).
- **Produced artifacts**:
  - `ZowiDesktop-<version>-windows-x86_64.zip` (portable, GUI + CLI)
  - `ZowiDesktop-<version>-setup-x64.exe` (Inno Setup installer)

To run it: **Actions → Windows CI → Run workflow**. When it finishes, download
the two artifacts. To attach them to a manual GitHub Release, place them in
`dist/`, then run:

```bash
./packaging/create-gh-release.sh --with-apt
```

## Windows native Bluetooth (bt_native)

For full Bluetooth support on Windows, a native DLL can be compiled on a Windows
machine with Visual Studio and placed next to `ZowiDesktop.exe`. The app
auto-detects it at startup.

See: [packaging/windows/bt_native/README.md](packaging/windows/bt_native/README.md)
