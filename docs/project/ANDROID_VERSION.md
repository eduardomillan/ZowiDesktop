# ANDROID_VERSION.md — Porting the `zowi_core` logic to Android / Flutter

## Table of contents

- [Goal](#goal)
- [What `src/core` is (and is not)](#what-srccore-is-and-is-not)
- [What is NOT in core (stays host-specific)](#what-is-not-in-core-stays-host-specific)
- [The refactor (merged) — making core Qt-free](#the-refactor-merged--making-core-qt-free)
  - [1. `translation_engine` — injectable loader + log sink](#1-translation_engine--injectable-loader--log-sink)
  - [2. `session_store` — injectable config directory](#2-session_store--injectable-config-directory)
  - [3. Removed orphan `DeviceInfo.h`](#3-removed-orphan-deviceinfoh)
  - [4. `src/core/CMakeLists.txt` — no Qt](#4-srccorecmakeliststxt--no-qt)
- [Verification after the refactor](#verification-after-the-refactor)
- [Integration routes (future)](#integration-routes-future)
- [Scope notes / decisions](#scope-notes--decisions)
- [See also](#see-also)

> This document records the analysis of reusing ZowiDesktop's `src/core/`
> business logic in a mobile app (native Android or Flutter), and the refactor
> that was applied to `src/core` to make it 100 % platform-independent and
> framework-free (Qt-free). The refactor is **already merged in this repo**; the
> mobile consumers here are a future project.

## Goal

Answer: *Can the logic programmed in `src/core` be reused by an Android or
Flutter application?*

**Short answer:** yes — the core is deliberately built as a Qt-free C++ library
and, after the refactor documented below, it compiles and tests with **no Qt at
all**. What is *not* portable is the transport layer (Bluetooth/USB backends),
which stays host-specific by design.

## What `src/core` is (and is not)

`src/core` (`Zowi::core`, static library) implements the framework-free business
logic:

| Module | Responsibility | Portability |
|---|---|---|
| `robot_commands` | Builds firmware command strings (`M`, `K`, `L`, `S`, `C`, `G`, melodies, mouths) | ✅ std C++ |
| `message_parser` | Tokenises the robot stream (ACK `A`, battery `B`, alarms `E/I`) | ✅ std C++ |
| `movement_sequencer` | State machine that times multi-command sequences | ✅ std C++ |
| `calibration_session` | Servo trims/grades state machine | ✅ std C++ |
| `robot_state` | Active-session state | ✅ std C++ |
| `config_store` | JSON key/value config | ✅ std C++ |
| `session_store` | Persistent JSON session (`getDataDir()`-injectable) | ✅ std C++ |
| `translation_engine` | i18n loading/translation | ✅ std C++ (see loader below) |

External dependencies: **none beyond `nlohmann_json`** (header-only) and
`cxx_std_20`. In particular `src/core` has **no Qt linkage**, no QObject, no
QML, and no platform I/O beyond `std::filesystem` / `std::ifstream`.

The full test suite lives in `src/core/tests/` and only links `Zowi::core`.

## What is NOT in core (stays host-specific)

- **Bluetooth**: `src/backends/bt_qt` (BlueZ/D-Bus, Linux), `bt_native` (WinRT),
  `bt_serial` (POSIX TTY), `bt_serial_win` (Win32). None are portable to
  Android/iOS. The core only defines the interface `zowi::BluetoothApi`
  (`src/core/include/zowi/bluetooth_api.h`) with `std::function` callbacks, so a
  mobile host implements the transport itself (e.g. Android `BluetoothSocket`
  SPP, or `flutter_blue_plus` in Flutter).
- **Firmware flashing**: `src/firmware/stk500v1` is std C++ (portable), but the
  `BootloaderTransport` (`send`/`receive`/`pump`/`progress`) is injected by the
  host. On Android the historical flow is raw-HEX over SPP
  (`zowiRawHexUploadFirmware`).

## The refactor (merged) — making core Qt-free

Before this change `src/core` had one Qt dependency: `translation_engine`
(`QFile`, `QTextStream`, `qDebug` + the compiled-in `:/i18n/` Qt resource). The
refactor removed it completely:

### 1. `translation_engine` — injectable loader + log sink

- `readJson` now parses a `std::string` (read via `std::ifstream`) instead of
  a `QFile`, keeping the exactly same JSON shape.
- The default loader resolves `i18n/zowi_<locale>.json` from the filesystem
  (under `resourceBasePath`, or CWD when empty) — same behaviour as the old
  filesystem branch.
- The **Qt resource fallback** (`:/i18n/zowi_<locale>.json`, used by packaged
  desktop builds) moved to the hosts, which now *inject* a loader via the new
  `setTranslationLoader()`:
  - `src/gui/controllers/TranslatorController.cpp:qtTranslationLoader()` —
    filesystem first, then `:/i18n/`.
  - `src/cli/cli_commands.cpp:runTranslate()` — same strategy
    (the CLI statically bundles `i18n.qrc` as `CLI_I18N_RC`).
- Logs (`qInfo`/`qWarning`) are replaced by an injectable `LogCallback`
  (`setLogCallback(LogLevel, message)`); the GUI routes it back into Qt's
  logging (so it still reaches the daily log file), the CLI discards it.

New public API (`translation_engine.h`):
```cpp
enum class LogLevel { Info, Warning };
using TranslationLoader = std::function<std::string(const std::string &locale)>;
void setTranslationLoader(TranslationLoader loader);
using LogCallback = std::function<void(LogLevel, const std::string &message)>;
void setLogCallback(LogCallback cb);
```

### 2. `session_store` — injectable config directory

`resolveConfigPath()` used `$XDG_CONFIG_HOME` / `$HOME` / `$APPDATA`
(desktop-only). It now accepts an explicit `configDir`:

```cpp
SessionStore(const std::string &organization = "ZowiDesktop",
             const std::string &application = "ZowiApp",
             const std::string &configDir = "");
```

Empty `configDir` keeps the old per-OS resolution (no desktop behaviour
change). Android passes `context.getDataDir()`, Flutter passes the
`path_provider` result.

### 3. Removed orphan `DeviceInfo.h`

`src/core/DeviceInfo.h` was a stale duplicate using `QString`/`QHash`. The real
type is `zowi::DeviceInfo` (`include/zowi/device_info.h`, `std::string`). The
Qt variant was unreferenced and is deleted.

### 4. `src/core/CMakeLists.txt` — no Qt

- Dropped `PRIVATE Qt${ZOWI_QT_VERSION}::Core` (the only Qt linkage in core).
- Kept `nlohmann_json` as the sole public dependency.
- Top-level `CMakeLists.txt`: `find_package(Qt Core)` is now conditional on
  `ZOWI_BUILD_GUI OR ZOWI_BUILD_CLI`, so a **core + tests build needs no Qt**:

```console
cmake -S . -B build -DZOWI_BUILD_GUI=OFF -DZOWI_BUILD_CLI=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Verification after the refactor

- Core-only build (`-DZOWI_BUILD_GUI=OFF -DZOWI_BUILD_CLI=OFF`) on a machine
  **without Qt available**: all 8 core tests pass ("100 % tests passed").
- Full build (GUI + CLI, Qt 6): compiles clean; all 9 tests pass including the
  CLI black-box suite; `zowi_cli translate` still resolves es_ES/en_US/ca_ES via
  the Qt-aware loader (filesystem + qrc fallback).

## Integration routes (future)

- **Native Android (Kotlin/Java):** build `zowi_core` as a static library with
  **CMake + NDK**, expose it through a thin `extern "C"`/JNI layer. Reuses the
  fully tested logic (movement sequencing, message parsing, calibration,
  commands) without re-implementing it.
- **Flutter:** bundle the static library and bind it via **`dart:ffi`**, usually
  behind a small C shim (C++ spans/`std::string` do not cross FFI directly).
  Core tests can run in CI (host side), as they do today.
- In both cases the only new host-specific code needed is a `BluetoothApi`
  implementation (SPP) and, optionally, a `BootloaderTransport` for flashing.

## Scope notes / decisions

- The mobile port itself (JNI/FFI wrapper, new transport) is **not** started;
  this repo stays the desktop app.
- Keep the i18n JSON format and the `TranslationListener`-style `onChanged`
  callback unchanged — the `TranslationEngine` API is now stable and
  framework-free, so a mobile host can reuse the same `.ts`-derived `zowi_*.json`
  files.

## See also

- [ARCHITECTURE.md](ARCHITECTURE.md) — layer overview and `zowi_core` role.
- [FIRMWARE_HOWTO.md](FIRMWARE_HOWTO.md) — STK500v1 / raw-HEX flashing and the
  injected `BootloaderTransport`.
- `src/core/CMakeLists.txt`, `src/core/include/zowi/` — current core surface.
- [BUILD.md](BUILD.md) — CMake options (`ZOWI_BUILD_GUI`, `ZOWI_BUILD_CLI`).