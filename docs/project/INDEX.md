# docs/project — Documentation Index

This directory documents the architecture, building, releasing and day-to-day
operation of Zowi Desktop. Start here, then follow the links. All documents are
in English.

## Quick start

- New to the project? Read [ARCHITECTURE.md](ARCHITECTURE.md) first, then [PLANNING.md](PLANNING.md).
- Working on the CLI? See [ZOWI_CLI_HOWTO.md](ZOWI_CLI_HOWTO.md) and [ZOWI_CLI_SHELL.md](ZOWI_CLI_SHELL.md).
- Building or packaging? See [BUILD.md](BUILD.md) and [RELEASE.md](RELEASE.md).
- Working with the Zowi robot's firmware? See [FIRMWARE_HOWTO.md](FIRMWARE_HOWTO.md) and [ZOWILIBS.md](ZOWILIBS.md).

## Files

| File | Summary |
|------|---------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | High-level design: **API + CLI + Frontend** layout, layers (`zowi_core`, backends, GUI controllers, CLI), build targets, data flow, dependencies, QML navigation/hot-reload, testing, and the evolution from the original MVVM design. |
| [PLANNING.md](PLANNING.md) | Living roadmap: release plan, current architecture thumbnail, implemented milestones (M1–M7) and future ones (M8, M9, future ideas), plus testing strategy and technical notes. |
| [BUILD.md](BUILD.md) | Building from source on Linux and Windows: prerequisites, Git settings, quick start, build targets, CMake options, platform builds (AppImage, `.deb`), CI workflows, `windeployqt`, Qt 5 vs Qt 6 notes. |
| [RELEASE.md](RELEASE.md) | End-to-end release guide: manual CI workflows (Linux CI, Windows CI), artifact production (local or CI), `create-gh-release.sh`, and publishing the signed apt repository to `gh-pages`. |
| [FIRMWARE_HOWTO.md](FIRMWARE_HOWTO.md) | How firmware upload works: bootloader protocol implemented in-app (STK500v1/Optiboot and raw-HEX modes), no PlatformIO/avrdude, over Bluetooth — plus what is needed for full USB support. |
| [ZOWI_CLI_HOWTO.md](ZOWI_CLI_HOWTO.md) | User guide for `zowi_cli`: every subcommand (connect/session/config/translate/firmware/ports/behaviours…), examples, hardware notes and privileges. |
| [ZOWILIBS.md](ZOWILIBS.md) | Relationship with the external **zowiLibs** repo: which components come from it (firmware HEX files, protocol, libraries), the sync script, the simulator and privileges. |
| [win/WIN_HOWTO.md](win/WIN_HOWTO.md) | Windows support: the native backends (`bt_native` WinRT Bluetooth, `bt_serial_win` Win32 serial), CMake/runtime backend selection, and how to build on Windows. |
| [MVVM.md](MVVM.md) | Primer comparing MVC, MVP and MVVM, why Qt/QML fits MVVM naturally, and the restaurant analogy. Background for the current architecture choice. |
| [UPGRADING_TO_QT6.md](UPGRADING_TO_QT6.md) | The migration notes from Qt 5.15 to Qt 6.5.2, including API changes and dependency changes. |
| [DOWNGRADING_TO_QT5.md](DOWNGRADING_TO_QT5.md) | Building with Qt 5.15 instead of Qt 6.5: dependency swap and the changes required in the GUI and Bluetooth backend. |
| [ZOWI_CLI_SHELL.md](ZOWI_CLI_SHELL.md) | Design document for the CLI **shell** (interactive mode): motivation, UX spec, command set, technical design, limitations and the future daemon mode. |
| [ANIMATIONS.md](ANIMATIONS.md) | Animation formats supported in QML (GIF, sprite sheets, frames, SVG, WEBP, Lottie) and recommended approaches for the splash screen. |
| [ANDROID_PORT.md](ANDROID_PORT.md) | Reusing the `zowi_core` business logic in Android/Flutter: what is portable, the Qt-free refactor (injectable translation loader, config-dir injection), and integration routes via NDK/JNI or dart:ffi. |

## Related navigation

- Per-screen documentation lives in [screens/](screens/INDEX.md) (`SCREEN_*.md`, one file per QML screen).
- The signed apt repository, the website (`gh-pages`) and release artefacts *do not* belong to this directory; see [RELEASE.md](RELEASE.md).