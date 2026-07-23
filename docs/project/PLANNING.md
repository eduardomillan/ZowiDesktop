# Development Plan — Zowi Desktop

> This is a living document. Edit it to reflect the actual roadmap and priorities.

## Release Plan

| Version | Milestone | Description | Status |
|---------|-----------|-------------|--------|
| **0.1.0** | M1 | Initial release: desktop GUI and CLI, Bluetooth connection, behaviours, firmware, i18n (5 locales) | ✅ |
| **0.2.0** | M2 | Debian/Lliurex packaging + Wayland support | ✅ |
| **0.3.0** | M3 | Automated GitHub Releases, signed APT repo, translations embedded in binary | ✅ |
| **0.3.2** | M3 | Multi-distro .deb (jammy + noble), AppImage on older base | ✅ |
| **0.4.0** | M4 | USB firmware flashing, `ports` subcommand, CLI tests by transport, splash no-BT banner | ✅ |
| **0.5.0** | M5 | Firmware restore GUI (BT+USB), low-battery confirmation, `adivinawi` CLI, transport selection in GUI | ✅ |
| **0.6.0** | M6 | Transport situation state machine, automatic transport, persistent preference, DEV overlay, restore feedback | 🚧 |
| **0.7.0** | M7 | Zowi calibration (servo trims via `C`/`G` protocol commands) | |
| **0.8.0** | M7 | Face/mouth editor | |

## Architecture

```
src/
├── core/          # Qt-free C++20 static library (zowi::core)
│   ├── session_store       # Persistent key-value store (JSON)
│   ├── config_store        # Read-only config loader
│   ├── translation_engine  # JSON-based i18n (5 locales)
│   ├── robot_commands      # Firmware command builder (20 movements)
│   ├── bluetooth_api       # Abstract backend interface
│   ├── protocol            # Firmware framing (&&cmd value%%)
│   ├── device_info         # Device struct (name, address, rssi)
│   └── transport_constants # usb/bt transport identifiers
├── backends/
│   ├── bt_qt/     # Bluetooth SPP via Qt + BlueZ D-Bus
│   └── bt_serial/ # USB/serial TTY backend
├── firmware/      # STK500v1 protocol + bundled .hex files
├── cli/           # CLI consumer (zowi_cli) — 13 subcommands
├── gui/           # Qt/QML GUI consumer — 4 controllers
└── views/         # QML screens (10) + components (5)
```

- `zowi::core` is intentionally Qt-free (except `translation_engine` which uses `QFile`).
- GUI and CLI share `core` and `backends` but are independent consumers.
- Two Bluetooth backends implement `BluetoothApi`: BlueZ D-Bus (SPP) and POSIX termios (serial).

## Milestones

### M1 — Initial release ✅
- [x] Desktop GUI and CLI to connect to Zowi over Bluetooth
- [x] Multilingual support (5 locales)
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

### M6 — Transport intelligence + gamepad 🚧
- [x] Automatic transport detection and switching
- [x] USB hotplug awareness
- [x] Persistent transport tied to device registration
- [x] Developer diagnostics overlay
- [x] Gamepad control screen (PadScreen) with directional pad and action buttons
- [X] Real-time command sending and speed control validation
- [ ] Compile the bt library for Windows
- [ ] Test the Windows version and fix bugs

### M7 - Calibration and mouth/gestures in gamepad
- [ ] Zowi calibration (servo trims)
- [ ] Gamepad mouth control
- [ ] Gamepad gestures control

### Future milestones
- [ ] Face/mouth editor
- [ ] Pre-programmed modes (demo, guardian, dancing, sound)
- [ ] Visual block editor
- [ ] Projects & tutorials section
- [ ] Polish & i18n (keyboard shortcuts, responsive layout)

## Testing

- **Core unit tests** (`src/core/tests/`): `test_session_store`, `test_config_store`, `test_translation_engine`, `test_robot_commands` — link only `zowi::core`, no Qt dependency.
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
