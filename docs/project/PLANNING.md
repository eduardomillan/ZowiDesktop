# Development Plan — Zowi Desktop

> This is a living document. Edit it to reflect the actual roadmap and priorities.

## Table of contents

- [Release Plan](#release-plan)
- [Architecture](#architecture)
- [Milestones](#milestones)
  - [M1 — Initial release ✅](#m1--initial-release-)
  - [M2 — Debian packaging + Wayland ✅](#m2--debian-packaging--wayland-)
  - [M3 — Automated releases + multi-distro ✅](#m3--automated-releases--multi-distro-)
  - [M4 — USB support ✅](#m4--usb-support-)
  - [M5 — Firmware restore GUI + transport selection ✅](#m5--firmware-restore-gui--transport-selection-)
  - [M6 — Transport intelligence + gamepad ✅](#m6--transport-intelligence--gamepad-)
  - [M7 - Calibration and mouth/gestures in gamepad + editor ✅](#m7--calibration-and-mouthgestures-in-gamepad--editor-)
  - [M8 - Basic projects](#m8--basic-projects)
  - [M9 - Advanced projects](#m9--advanced-projects)
  - [Future milestones](#future-milestones)
- [Testing](#testing)
- [Technical notes](#technical-notes)
- [Repository strategy (monorepo vs. multiple repos)](#repository-strategy-monorepo-vs-multiple-repos)

## Release Plan

Status: 
- ✅ Done 
- 🚧 Under development
- 🕒 Planned


| Version | Milestone | Description | Status |
|---------|-----------|-------------|--------|
| **0.1.0** | M1 | Initial release: desktop GUI and CLI, Bluetooth connection, behaviours, firmware, i18n (all locales) | ✅ |
| **0.2.0** | M2 | Debian/Lliurex packaging + Wayland support | ✅ |
| **0.3.0** | M3 | Automated GitHub Releases, signed APT repo, translations embedded in binary | ✅ |
| **0.3.2** | M3 | Multi-distro .deb (jammy + noble), AppImage on older base | ✅ |
| **0.4.0** | M4 | USB firmware flashing, `ports` subcommand, CLI tests by transport, splash no-BT banner | ✅ |
| **0.5.0** | M5 | Firmware restore GUI (BT+USB), low-battery confirmation, `adivinawi` CLI, transport selection in GUI | ✅ |
| **0.6.0** | M6 | Transport situation state machine, automatic transport, persistent preference, DEV overlay, restore feedback | ✅ |
| **0.7.0** | M7 | Zowi calibration (servo trims via `C`/`G` protocol commands) | ✅ |
| **0.8.0** | M8 | Basic projects | 🚧 |
| **0.9.0** | M8 | Advanced projects | 🕒 |
| **0.10.0** | M8 | Design improvements | 🕒 |

## Architecture

```
src/
├── core/          # Qt-free C++20 static library (zowi::core)
│   ├── session_store       # Persistent key-value store (JSON)
│   ├── config_store        # Read-only config loader
│   ├── translation_engine  # JSON-based i18n (all locales)
│   ├── robot_commands      # Firmware command builder (20 movements)
│   ├── robot_state         # Cached robot identity/battery state
│   ├── message_parser      # Parses incoming robot stream messages
│   ├── movement_sequencer  # Drives sequences of timed movements
│   ├── calibration_session # Servo trim calibration state machine
│   ├── bluetooth_api       # Abstract backend interface
│   ├── protocol            # Firmware framing (&&cmd value%%)
│   ├── device_info         # Device struct (name, address, rssi)
│   └── transport_constants # usb/bt transport identifiers
├── backends/
│   ├── bt_qt/       # Bluetooth SPP via Qt + BlueZ D-Bus (POSIX)
│   ├── bt_native/   # WinRT Bluetooth backend (Windows only)
│   ├── bt_serial/   # USB/serial TTY backend (POSIX)
│   └── bt_serial_win/ # Win32 USB/serial backend (Windows only)
├── firmware/      # STK500v1 protocol + bundled .hex files
├── cli/           # CLI consumer (zowi_cli) — 19 subcommands
├── gui/           # Qt/QML GUI consumer — 6 controllers
└── views/         # QML screens (13) + template + components (6)
```

- `zowi::core` is 100 % Qt-free: it has no Qt dependency at all, and its only
  third-party dependency is `nlohmann/json`. Platform-dependent plumbing (the
  translation file loader, the `SessionStore` config directory, log sinks) is
  injected by the consumers — see [ANDROID_PORT.md](ANDROID_PORT.md).
- GUI and CLI share `core` and `backends` but are independent consumers.
- Four backends implement `BluetoothApi`: `bt_qt` (BlueZ D-Bus SPP, POSIX),
  `bt_native` (WinRT, Windows), `bt_serial` (POSIX termios) and
  `bt_serial_win` (Win32 serial).

## Milestones

### M1 — Initial release ✅
- [x] Desktop GUI and CLI to connect to Zowi over Bluetooth
- [x] Multilingual support (all locales)
- [x] Persistent session and device configuration
- [x] Device discovery and pairing flow
- [x] Connection status and battery indicators
- [x] Basic firmware management (restore, alarm)
- [x] Windows and Linux builds (AppImage + portable)

### M2 — Debian packaging + Wayland ✅
- [x] Official Debian/Lliurex package (`zowi-desktop`)
- [x] Signed APT repository for easy install and updates
- [x] Wayland session support
- [x] GPG-signed repository metadata

### M3 — Automated releases + multi-distro ✅
- [x] Automated release pipeline (tag → AppImage + `.deb`)
- [x] Embedded translations in application binary
- [x] Multi-distro support (Ubuntu 22.04 / 24.04)
- [x] Forward-compatible AppImage on older base

### M4 — USB support ✅
- [x] USB/serial connection as alternative to Bluetooth
- [x] USB port enumeration and auto-detection
- [x] Firmware flashing over USB
- [x] Organised test suite by transport type
- [x] Splash screen guidance when no Bluetooth available

### M5 — Firmware restore GUI + transport selection ✅
- [x] Restore firmware from GUI (Bluetooth + USB)
- [x] Battery safety check before restore
- [x] Game firmware install (Adivinawi)
- [x] User-selectable transport in Settings
- [x] Visual feedback during restore (progress bar + status)

### M6 — Transport intelligence + gamepad ✅
- [x] Automatic transport detection and switching
- [x] USB hotplug awareness
- [x] Persistent transport tied to device registration
- [x] Developer diagnostics overlay
- [x] Gamepad control screen (PadScreen) with directional pad and action buttons
- [x] Real-time command sending and speed control validation
- [x] Compile the bt library for Windows
- [x] Test the Windows version and fix bugs

### M7 - Calibration and mouth/gestures in gamepad + editor ✅
- [x] Zowi calibration (servo trims)
- [x] Gamepad mouth control
- [x] Gamepad gestures control
- [x] Custom mouth editor (draw/edit arbitrary LED-matrix patterns)

### M8 - Basic projects
- [x] **Move objects** — implemented (ProjectMoveScreen, QuizComponent, ProjectsStore, ProjectsController, ProjectsPreferencesStore, projects.qrc, i18n)
- [ ] The shape of Zowi and biped robots
- [ ] Zowi eyes and ultrasounds
- [ ] The Zowi legs and servos
- [ ] The gravity and calibration
- [ ] **Projects CLI commands** — `project list/show/reset/prefs` (uses core ProjectsStore/ProjectsPreferencesStore, no Qt)

### M9 - Advanced projects
- [ ] Robot dancing and choreography (sequence programming in home screen)
- [ ] The alarm robot
- [ ] Fortune-telling robot
- [ ] Bitbloq I: Hello World
- [ ] Bitbloq II: Sensors

### Future milestones
- [ ] Game: Visual block editor
- [ ] Game: Memory
- [ ] Game: Draw the mouth
- [ ] Achievements layer
- [ ] Testing, validating and improving
- [ ] Adapt the software to other robots (Otto with Arduino or ESP32)

## Testing

- **Core unit tests** (`src/core/tests/`): `test_session_store`, `test_config_store`,
  `test_translation_engine`, `test_robot_commands`, `test_calibration_session`,
  `test_movement_sequencer`, `test_message_parser`, `test_robot_state` — link only
  `zowi::core`, no Qt dependency.
- **CLI integration tests** (`src/cli/tests/`): Bluetooth (6 scripts) and USB (8 scripts) — require real hardware.
- **QML preview scripts** (`src/views/tests/`): Shell scripts to launch individual screens.
- Run tests: `ctest --test-dir build --output-on-failure`

## Technical notes

- Bluetooth Classic SPP only — BLE is not supported by Zowi's HC-06/HC-05 module.
- Firmware flashing uses the same STK500v1 protocol as the original Android app.
- Translations are loaded from `.json` files at runtime (no `lrelease` needed). Fallback: English.
- All robot images and UI assets come from the original Android project (`drawable-xxxhdpi`).
- GUI debug builds load QML from disk with hot-reload; release builds use `resources.qrc`.
- Runtime logs: `qDebug`/`qWarning` mirrored to stderr + per-day log file at `AppDataLocation`.

## Repository strategy (monorepo vs. multiple repos)

**Recommendation: keep a single monorepo with clean module boundaries; consider
splitting only if/when real Android/Web consumers appear.**

The repository is already a monorepo with decoupled modules: `zowi_core` is a
Qt-free static library, and the GUI (`-DZOWI_BUILD_GUI=OFF`) and CLI
(`-DZOWI_BUILD_CLI=OFF`) build independently. The Qt-free refactor
(`TranslationEngine`/`SessionStore` injection) makes a future split *possible*,
not *necessary* today.

Why a full split hurts more than it helps for a single maintainer:

- **Versioning and releases**: `core` would need its own semver/tags/CI
  publishing, and every change would force version bumps and release
  coordination across cli/gui/android/web. The release flow (AppImage, `.deb`,
  apt repo, gh-pages) is coupled to this repo.
- **Shared non-core code**: the GUI and CLI share `src/backends/` (`bt_qt`,
  `bt_serial`, `bt_serial_win`, `bt_native`) and `src/firmware/`. Splitting
  cli/gui into separate repos would either duplicate that code or require a
  fifth "backends" repo. Android and Web do not use the backends (they bring
  their own native Bluetooth/wasm), so `core` is the only genuinely shared
  piece.
- **Cross-repo coordination**: PRs touching several repos, submodules/
  FetchContent between them, more workflows. For a single developer the
  monorepo wins on simplicity; a split pays off with distinct teams or release
  cadences.

The one split that makes sense (deferred): extract **`zowi_core` alone**
(optionally with `protocol.h`/firmware) into its own repo consumable via
`FetchContent`/submodule. It is clean because core depends on nothing. Do this
only once a real Android/Web consumer exists — today all consumers are in-repo,
so a separate repo would be pure overhead.

Suggested split, if it ever happens:

| Repo | Content | Consumed by |
|---|---|---|
| `zowi-core` | `src/core/` (API, protocol) | cli, gui, android, web |
| `ZowiDesktop` | GUI + CLI + backends + packaging | — |
| `ZowiAndroid` | Java/Kotlin app + JNI wrapper (core as submodule) | — |
| (web) | wasm/JS wrapper over core (core as submodule) | — |

Decision trigger: concrete Android/Web port plans, additional maintainers/teams,
or a problem the monorepo cannot solve (slower builds, permissions, history,
releases).
