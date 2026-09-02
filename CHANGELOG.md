# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Servo calibration.** New `calibrate` CLI subcommand for interactive servo-trim
  calibration (wizard with arrow keys) or direct mode (`--yl/--yr/--rl/--rr`).
  New GUI `CalibrationScreen` with a 4-step wizard (WARNING → LEGS → FEET → CHECK)
  accessible from Settings. Both share a Qt-free `CalibrationSession` core module
  that owns the trim state, clamps (±60°), command generation and the one-G-in-flight
  debounce policy.
- **Extended robot commands.** `robot_commands.h` now exposes `MouthId` (31 mouth
  patterns), `GestureId` (13 gestures) and `MelodyId` (19 melodies) enums, plus
  `commandMouth()`, `commandMouthById()`, `commandGesture(GestureId)` and
  `commandSing()` functions. A new `CommandsController` exposes all robot commands
  to QML, replacing hardcoded protocol strings in `PadScreen`.

### Changed
- **PadScreen refactor.** All movement/action buttons now use `Commands.xxx()`
  methods instead of hardcoded protocol strings (`"M 3 " + speed + "\r"` →
  `Commands.turnLeft(speed)`), making the code type-safe and testable.
- **Calibration button styling.** CalibrationScreen action buttons now use the
  same pill style as Welcome/Wizard/WizardFound screens (200×56 in rows, 260×56
  for solo buttons, 190×56 for the 3-button CHECK step), with consistent
  `radius: 28` and `font.pixelSize: 16`.
- **`dev_mode` default.** The `dev_mode` key in `config.json` is now `false` by
  default (was `true`).
- **Renamed `ZOWI_DEV` environment variable to `DEV_MODE`.** The env var
  that enables dev mode and the dev overlay is now `DEV_MODE` (accepted
  values: `1`, `true`, `on`, case-insensitive). The `dev_mode` key in
  `config.json` is unchanged.

### Fixed
- **Calibration entry caused an unwanted leg sweep.** Entering calibration
  (GUI or CLI) used to send `C 0 0 0 0` followed immediately by
  `G 90 90 90 90`. Because the firmware's `receiveTrims` handler calls
  `zowi.home()` internally, this produced two back-to-back moves to 90°
  with a detach/re-attach in between — causing the legs to sag, re-energise
  with a visible clunk, and appear to "collide at the rear" when swinging
  inward from an arbitrary posture. The entry sequence now sends a single
  `S` (stop) command, which invokes the firmware's `home()` exactly once,
  settling the robot cleanly to rest without the redundant second move.
- **Calibration state persisted between openings.** The global
  `CalibrationSessionController` kept its step/trims state between screen
  openings, so a previous calibration's step 3 would reappear on the next
  open. `CalibrationScreen` now resets to step 0 with zeroed trims on
  `Component.onCompleted` (respecting the preview's `PreviewStep` hook).

## [0.6.0] - 2026-09-01

### Added
- **Choose USB or Bluetooth in the GUI.** The desktop app is no longer locked
  to Bluetooth: a new *Connection* selector in **Settings** lets the user pick
  **Automatic**, **Bluetooth** or **USB cable**. The `RobotController` is
  now transport-agnostic and can drive either the Qt/BlueZ SPP backend or the
  serial/USB backend.
- **Automatic transport detection (hybrid).** In *Automatic* mode the app
  detects the best available connection at startup and preselects it, giving
  Bluetooth priority when a robot is found on a port. 
- **USB robot identification handshake.** Before treating a USB serial port as
  a robot, the app performs a lightweight `I` (program-id) handshake and only
  accepts ports that reply with a valid Zowi app id, avoiding false positives
  from unrelated serial devices. Because opening the port resets the robot
  (DTR), each port is probed at most once per session and only while
  disconnected.
- **USB hotplug awareness.** The app polls for the appearance/removal of USB
  ports (and Bluetooth adapters) and updates the UI automatically; a manual
  *Refresh* action is also available.
- **Active-transport badge** in the status bar (USB / Bluetooth) while
  connected, and a *Connect via USB* / *Refresh* quick action in Settings.
- **Persisted transport preference.** The selected transport is remembered
  across sessions (session store), honoured on next launch when still
  available. New `transport` and `usb_baud` defaults were added to
  `config.json`.
- **`adivinawi` CLI subcommand.** Install the bundled Adivinawi game firmware
  (`src/firmware/ZOWI_Adivinawi_v2.hex`) on the paired Zowi, mirroring the
  existing `alarm` command. Supports the same options (`--firmware`,
  `--timeout`, `--battery-timeout`, `--force-low-battery`, `--protocol`,
  `--tty`, `--baud`, `--address`, `--backend`) over Bluetooth or USB, and can
  be reverted with `restore`.
- Bluetooth and USB test scripts for the Adivinawi install flow
  (`src/cli/tests/{bt,usb}/test_install_adivinawi.sh`), wired into `run_all.sh`.
- **Restore base firmware from the GUI over Bluetooth AND USB.** The
  *Restore firmware* action in **Settings** now flashes `ZOWI_BASE_v2.hex`
  directly from the bundled resources (`app.qrc`, `:/firmware/...`). For USB it
  reopens the TTY at the Optiboot bootloader baud (`usb_bootloader_baud`,
  default `115200`), pulses the DTR reset and uploads via STK500v1, then
  restores the operating baud (`usb_baud`) and reconnects to the running
  firmware. For Bluetooth it triggers the STATE-pin reset by reconnecting.
- **Restore feedback.** While a restore runs, the *Connection* section and the
  restore option are disabled; the outcome is reported in the message bar
  (`restore_started` / `restore_success` / `restore_failed`) in the 5 supported
  locales, and a live progress bar (yellow text above, left-to-right fill)
  occupies the message-bar position during the upload.
- **Blocking transport switch in Settings.** Picking **Automatic** / **Bluetooth**
  / **USB cable** in the *Connection* selector now tears down the live link,
  switches the backend, and reconnects with a configurable timeout
  (`transport_timeout`, default `1500 ms`). If the chosen transport cannot
  connect in time, it reports an error and reverts to the previous transport.
  The whole Settings screen is disabled during the switch.
- **Transport auto-fallback on (re)registration / forget / reset.** Starting the
  pairing wizard, forgetting the Zowi in Settings, or resetting from the splash
  screen now forces the transport back to **Automatic**
  (`setTransportPreference`).
- **Rename lock after pairing.** `WizardRenameScreen` disables the name field
  and rename button for a configurable period (`rename_lock_ms`, default
  `3000 ms`) after appearing, giving the robot time to finish its welcome
  gesture before the user types/confirms.
- **DEV overlay improvements (diagnostic).** The `DevOverlay` now opens expanded
  by default, word-wraps its text, is resizable from its right / bottom / corner
  edges, and has a **Copy** button that places the full log on the clipboard.
- **Pre-upload low-battery confirmation on restore.** Before the upload, the
  running firmware's reported battery level is checked (mirroring the CLI's
  `--force-low-battery` flow); if it is below the configurable
  `restore_low_battery_threshold` (default `50`) a confirmation dialog is shown
  over the progress bar and the restore is deferred. **Continue** proceeds with
  the reset+upload; **Cancel** aborts without touching the robot. New signal
  `firmwareRestoreBatteryLow(level)` and the `confirmRestoreBattery(bool)` slot
  back this handshake.
- **Native Windows support.** The Windows port now uses a dedicated C++/WinRT
  Bluetooth backend (`NativeBluetoothBackend`) with classic device discovery,
  automatic pairing (`PIN 1234`) and an SPP/RFCOMM `StreamSocket`, replacing Qt
  Bluetooth / BlueZ on Windows; the `bt_native` backend is shared by the GUI and
  the CLI. USB runs over the WinSerial backend (COM ports), with DTR-driven
  resets suppressed while enumerating and polling ports. A `build.bat`, portable
  zip, Inno Setup installer and a manual release flow for the Windows artifacts
  complete the platform support.
- **New PadScreen.** Drives the robot with click-and-hold controls (directional
  pad, turns, side action buttons) and an entry point to the Animations view;
  the Home screen buttons were reorganised into a single centred row.
- **CLI `control` subcommand.** Interactive driving with arrow keys / WASD, Q/E
  turns and speed presets, also available over USB with `--backend usb`.
- **ARM action movements in core.** Added `bend`, `shake_leg`, `updown`,
  `jitter`, `swing`, `flapping`, `crusaito`, and other action commands.
- **CLI black-box test suite.** `scripts/test/run-cli-blackbox*.sh` covers the
  CLI end to end and is wired into `run-tests.sh` and CI.
- **Per-day log file.** The GUI logs to `ZowiDesktop-YYYY-MM-DD.log` under the
  platform AppData location (e.g. `~/.local/share/ZowiDesktop/`); the console
  output is filtered by `log_level` while the file keeps the full history.
- **Locales persisted across sessions** via the session store.
- **Bluetooth-off detection.** `hasAdapter()` now uses the WinRT Radios API and
  only reports an adapter when a Bluetooth radio is actually powered on.
- **USB auto-reconnect.** When a previously used USB port reappears the app
  reconnects automatically, and USB disconnects are detected.

### Changed
- **Transport is no longer user-selectable.** The *Connection* selector in
  **Settings** (Automatic / Bluetooth / USB) has been replaced by a status panel
  driven by a `RobotController` situation state machine
  (`Demo` / `Unregistered` / `Connecting` / `Connected` / `Disconnected` /
  `TransportLost`). The app now picks the best available transport on its own and
  offers only contextual actions (retry, register, forget & reconfigure, connect
  via USB, refresh).
- **The registered transport is now tied to the Zowi registration.**
  `RobotController` persists `activeZowiTransport` (bt/usb) when a registered
  Zowi connects, so switching transports requires *forgetting* the Zowi.
- **Renamed `BluetoothController` → `RobotController`** and the QML context
  property `Bluetooth` → `Robot`, reflecting that the controller is
  transport-agnostic (covers connection, registration, firmware restore and the
  new situation state machine).
- **Window title now shows the app version.** Format:
  `ZowiDesktop - {version} - {screen name}`.
- **DEV overlay** no longer duplicates the robot name/address line.
- **`[bt_native]` messages now go through Qt logging.** The per-second
  send/receive trace (GUI and CLI) only appears when `log_level` is `debug`;
  real errors surface as `qWarning`. The CLI installs a matching message handler
  that mirrors warnings/errors and only prints debug output at `log_level=debug`.
- **Auto-transport refinements.** Auto mode honours the registered transport,
  forces the USB backend when Bluetooth is unavailable, and shows the USB
  disconnect hint only for a confirmed Zowi over USB.
- **Semantic colors unified** in `config.json`; transport values are normalised
  to the `bt` / `usb` constants shared by core.
- **ScanScreen** hides the device list until a scan starts and appends the robot
  name to status messages.
- **StatusBar** distinguishes `TransportLost` from `Disconnected`, and its
  visibility is fixed.
- `config.json` gains `usb_bootloader_baud` (`115200`), `transport_timeout`
  (`1500`), `rename_lock_ms` (`5000`), `restore_low_battery_threshold` (`50`),
  `restore_simulate_low_battery` (testing aid) and `factory_firmware_path`.
- Transport persistence now goes through the shared `SessionController` store so
  the DEV *SESSION* panel reflects the live `transport` value.
- `scripts/sync_firmware_from_zowiLibs.sh` now matches the current zowiLibs
  layout (`code/base/` and `code/games/<name>/<name>.hex`) and copies all game
  firmware, including Adivinawi.
- The "no Bluetooth" splash banner now only appears when **neither** a
  Bluetooth adapter **nor** a USB robot is available, and its message guides
  the user to plug in via USB or enable Bluetooth.
- Home-screen auto-connect on launch now connects over USB when USB is the
  active transport and a robot is present, falling back to the saved Bluetooth
  device otherwise.

### Fixed
- **Windows fixes.** The CLI interactive `control` mode, SerialDevice port
  matching and the portable build were fixed; the GUI no longer hangs on close
  and *Forget Zowi* no longer crashes (the transport is closed, not the reader,
  to safely interrupt `LoadAsync`); the Inno Setup installer script was
  corrected.
- **Connection-attempt timeout falls back to Demo.** A connection attempt that
  outlives `connect_timeout` no longer leaves the UI stuck in *Connecting…*; it
  drops into Demo and keeps probing in the background.
- **USB robot identity verified against the registered Zowi.** A USB port is
  only treated as the registered robot when the reported name matches.
- **Wait for the robot name** before applying the *already named* rename skip.
- **USB restore failed silently.** `restoreFirmware` trusted `m_deviceAddress`,
  which the serial backend clears on every reconnect, so the USB target was
  empty and the upload bailed out before even reopening the port. It now falls
  back to `m_usbPort` / `m_knownUsbPorts`, and `onConnectionChanged(true)`
  restores `m_deviceAddress` from the USB port.
- **`qrc:/` firmware path not openable.** `QFile` does not understand the
  `qrc:/` URL syntax used by QML; the path is now normalised to the `:/`
  resource syntax before extracting the HEX to a temporary file (the temp file
  is kept alive for the duration of the upload).
- **Remaining i18n keys translated** in `bg_BG`, `ca_ES` and `fr_FR`
  (connection, transport and USB labels), and the DevOverlay close-button font
  size no longer references an undefined property.

## [0.5.0] - 2026-07-18

### Added
- **Restore base firmware from the GUI over Bluetooth AND USB.** The
  *Restore firmware* action in **Settings** now flashes `ZOWI_BASE_v2.hex`
  directly from the bundled resources. For USB it reopens the TTY at the
  Optiboot bootloader baud, pulses the DTR reset and uploads via STK500v1,
  then restores the operating baud and reconnects. For Bluetooth it triggers
  the STATE-pin reset by reconnecting.
- **Pre-upload low-battery confirmation on restore.** Before the upload, the
  running firmware's reported battery level is checked; if it is below the
  configurable `restore_low_battery_threshold` (default `50`) a confirmation
  dialog is shown. **Continue** proceeds with the reset+upload; **Cancel**
  aborts without touching the robot.
- **`adivinawi` CLI subcommand.** Install the bundled Adivinawi game firmware
  on the paired Zowi, mirroring the existing `alarm` command. Supports the
  same options over Bluetooth or USB, and can be reverted with `restore`.
- **USB/BT transport selection in the GUI.** A new *Connection* selector in
  **Settings** lets the user pick **Automatic**, **Bluetooth** or **USB cable**.
  The controller is now transport-agnostic and can drive either backend.

### Changed
- **Reorganised CLI.** `main.cpp` split into `state`, `util`, and `commands`
  modules for clearer separation of concerns.
- **Restore feedback.** While a restore runs, the *Connection* section and the
  restore option are disabled; the outcome is reported in the message bar and a
  live progress bar occupies the message-bar position during the upload.
- `scripts/sync_firmware_from_zowiLibs.sh` now matches the current zowiLibs
  layout and copies all game firmware, including Adivinawi.

## [0.4.0] - 2026-07-17

### Added
- **USB firmware flashing (no Bluetooth required).** A new `usb` backend mode
  (`--backend usb`) lets the CLI talk to the robot over a USB serial link
  (`/dev/ttyUSB*`, `/dev/ttyACM*`), so firmware can be uploaded on machines
  without a Bluetooth adapter. When no `--tty` is given, the first available
  port is auto-selected.
- **Configurable serial baud rate** via `--baud` on the `restore` and `alarm`
  commands (defaults to 9600 for the RFCOMM/ZUM bootloader; USB Optiboot
  typically uses 57600 or 115200).
- **New `ports` subcommand** that enumerates available USB serial ports, the
  USB analogue of the Bluetooth `scan` command.

### Changed
- **Reorganised Qt resources.** The single monolithic `resources.qrc` was
  split by domain into `views.qrc` (QML), `app.qrc` (config + app icon) and
  `images.qrc` (images, grouped by subfolder). The GUI now reuses the
  existing `i18n.qrc` instead of duplicating the translation list. Resource
  paths (`qrc:/...`) are unchanged, so no code was affected.
- **Reorganised CLI tests by transport.** The existing Bluetooth tests moved to
  `src/cli/tests/bt/`, and analogous USB tests were added under
  `src/cli/tests/usb/` (`test_ports.sh`, `test_install_alarm.sh`,
  `test_restore_factory_firmware.sh`, `test_usb_options.sh`). Each transport
  folder now has a `run_all.sh` runner. The USB testing workflow is documented
  in `docs/tests/ZOWI_CLI_HOWTO.md`.

### Fixed
- **AppImage release build failed with "required QML module(s) missing".**
  `QtQml.Base` and `QtQml.WorkerScript` (and the transitively-required
  `QtQml.Models`, `QtQuick.Templates`, `QtQuick.Shapes`) live in separate apt
  packages that were not installed in CI, so they could not be bundled. Added
  the missing `qml6-module-*` packages and made the AppImage QML bundling and
  verification steps resilient to the apt Qt layout (where `QtQml.Base` is
  declared in the top-level `QtQml/qmldir` rather than its own subdirectory).
- **Release builds (AppImage and `.deb`) failed to compile.** The GUI
  `zowi_screen_preview` target failed with a `QVariant` -> `QString` conversion
  error in `preview_main.cpp` (`QVariantMap::value()` returns a `QVariant`),
  breaking the AppImage and both Debian packages in CI. Added the missing
  `.toString()` conversion.
- **USB firmware flashing timed out right after connecting.** The
  firmware-flash flow reset its connection state and then waited for the
  connection callback to fire *again*, which only happens on Bluetooth (the
  STATE-pin reset causes a reconnect). On serial/USB the port is opened
  synchronously and never reconnects, so `restore`/`alarm` failed with
  "Could not connect to the robot within the timeout." The flow now seeds its
  connection state from the backend's actual `isConnected()` status.
- **AppImage failed to start on machines without a full Qt install.** The
  bundled AppImage was missing the transitively-imported `QtQml.WorkerScript`
  QML module (pulled in by `ListModel`/`ListView`), causing
  `module "QtQml.WorkerScript" is not installed` and the app not to launch.
  `create-appimage.sh` now explicitly bundles the essential `QtQml` modules
  (`Base`, `Models`, `WorkerScript`, `XmlListModel`) and verifies they are
  present before producing the image.

### Added
- The splash screen now shows an informative banner when no Bluetooth
  adapter is detected, letting the user know the app will run in demo mode
  only. Exposed via the new `Bluetooth.bluetoothAvailable` property.

## [0.3.2] - 2026-07-14

### Changed
- **Multi-distro packaging.** The Debian package is now built separately for
  **Lliurex 23 / Ubuntu 22.04 (jammy)** and **Lliurex 25 / Ubuntu
  24.04 (noble)** and published to two APT suites (`jammy` and
  `noble`) in the same signed repository, because Qt 6 gained the `t64`
  ABI suffix (and newer glibc/libstdc++) on noble, making a single
  `.deb` incompatible across both bases.
- The **AppImage** is built on the older 22.04 base (self-contained Qt
  6.2.4) so the single image runs on both Lliurex 23 and 25 via
  forward glibc compatibility.

### Fixed
- `Depends` used the wrong QML module name `qml6-module-qtquick2`;
  corrected to `qml6-module-qtquick` (Qt 6).

## [0.3.0] - 2026-07-14

### Added
- **Automated GitHub Releases.** Pushing a `v*` tag now builds and publishes,
  as release assets, the Linux AppImage
  (`ZowiDesktop-<version>-x86_64.AppImage`) and the Debian package
  (`zowi-desktop_<version>-1_amd64.deb`).
- **Signed APT repository for Lliurex / Debian / Ubuntu**, published to
  GitHub Pages and consumable with a single `deb` source line so the app can
  be installed and auto-updated with `apt`.
- **GPG-signed repository metadata** (`InRelease` / `Release.gpg`) plus a
  downloadable `keyring.gpg` for `signed-by` verification.
- **Wayland support.** The AppImage now bundles the Qt Wayland platform plugin
  (and its dependencies), and the `.deb` declares a dependency on
  `qt6-wayland`, so the application runs on Wayland sessions in addition to
  X11.

### Changed
- **Translations are now embedded in the application binary** (Qt resource
  `qrc`) with a filesystem fallback, so the GUI and CLI resolve their language
  files regardless of the working directory or installation layout.
- The release pipeline builds the AppImage against the distribution's Qt 6
  (still self-contained) instead of a separately downloaded toolchain.

### Fixed
- **i18n runtime resolution.** `zowi_<locale>.json` is loaded from the embedded
  resources first, then falls back to the filesystem, fixing missing
  translations when the app is launched from an arbitrary directory.
- The Debian package now builds fully offline using the system-provided
  `nlohmann-json3-dev` and `libcli11-dev`, with a FetchContent fallback kept
  for local development.
- The `.deb` build step now places its artifacts in `build/` correctly.

## [0.2.0] - 2026-07-14

### Added
- Initial Debian / Lliurex packaging (`zowi-desktop`): GUI and CLI built against
  the system Qt 6, with a signed APT repository published to GitHub Pages and
  Wayland support.

## [0.1.0] - 2026-07-13

### Added
- Initial release: desktop GUI and CLI to connect to a Zowi robot over
  Bluetooth, drive its behaviours, manage firmware, and a translation engine
  with the 5 supported locales.
