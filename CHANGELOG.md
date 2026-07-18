# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

> [!WARNING]
> **Estado: EN PRUEBAS.** Todo lo descrito a continuación pertenece a la rama
> `restore-base-firmware-gui` y **no está en `main` todavía**. El firmware base
> se restaura correctamente tanto por Bluetooth como por USB en la GUI, pero el
> flujo sigue en fase de validación manual (feedback de restauración, cambio de
> transporte bloqueante, bloqueo de renombrado tras emparejar). No se ha hecho
> merge ni release.

### Added
- **`adivinawi` CLI subcommand.** Install the bundled Adivinawi game firmware
  (`src/firmware/ZOWI_Adivinawi_v2.hex`) on the paired Zowi, mirroring the
  existing `alarm` command. Supports the same options (`--firmware`,
  `--timeout`, `--battery-timeout`, `--force-low-battery`, `--protocol`,
  `--tty`, `--baud`, `--address`, `--backend`) over Bluetooth or USB, and can
  be reverted with `restore`.
- Bluetooth and USB test scripts for the Adivinawi install flow
  (`src/cli/tests/{bt,usb}/test_install_adivinawi.sh`), wired into `run_all.sh`.

### Added (branch `restore-base-firmware-gui`, EN PRUEBAS)
- **Restore base firmware from the GUI over Bluetooth AND USB.** The
  *Restore firmware* action in **Settings** now flashes `ZOWI_BASE_v2.hex`
  directly from the bundled resources (`app.qrc`, `:/firmware/...`). For USB it
  reopens the TTY at the Optiboot bootloader baud (`usb_bootloader_baud`,
  default `115200`), pulses the DTR reset and uploads via STK500v1, then
  restores the operating baud (`usb_baud`) and reconnects to the running
  firmware. For Bluetooth it triggers the STATE-pin reset by reconnecting.
- **Phase-1 restore feedback.** While a restore runs, the *Connection* section
  and the restore option are disabled; the outcome is reported in the
  message bar (`restore_started` / `restore_success` / `restore_failed`) in the
  5 supported locales.
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
  `1500 ms`) after appearing, giving the robot time to finish its welcome
  gesture before the user types/confirms.
- **DEV overlay improvements (diagnostic).** The `DevOverlay` now opens expanded
  by default, word-wraps its text, is resizable from its right / bottom / corner
  edges, and has a **Copy** button that places the full log on the clipboard.
   These are intended as debugging aids during the testing phase.
- **Phase 3: dedicated restore worker thread + battery confirmation.** Only the
  blocking STK500 upload runs on a dedicated worker thread (the reset/reconnect
  and the post-upload battery check stay on the GUI thread, because the backend
  uses a `QSocketNotifier` that is not thread-safe — running the whole restore
  in a thread crashed with "Socket notifiers cannot be enabled or disabled from
  another thread"). The GUI thread keeps its event loop running during the
  upload, so the progress bar stays responsive instead of freezing.
  After a successful upload the running firmware reports its battery level
  (mirroring the CLI's `--force-low-battery` check); if it is below 50% a
  confirmation dialog is shown over the progress bar and the UI defers finishing
  until the user confirms or cancels. New signal `firmwareRestoreBatteryLow(level)`
  and the `confirmRestoreBattery(bool)` slot back this handshake.

### Changed (branch `restore-base-firmware-gui`, EN PRUEBAS)
- `config.json` gains `usb_bootloader_baud` (`115200`), `transport_timeout`
  (`1500`) and `rename_lock_ms` (`1500`), all configurable.
- Transport persistence now goes through the shared `SessionController` store so
  the DEV *SESSION* panel reflects the live `transport` value.

### Fixed (branch `restore-base-firmware-gui`, EN PRUEBAS)
- **USB restore failed silently.** `restoreFirmware` trusted `m_deviceAddress`,
  which the serial backend clears on every reconnect, so the USB target was
  empty and the upload bailed out before even reopening the port. It now falls
  back to `m_usbPort` / `m_knownUsbPorts`, and `onConnectionChanged(true)`
  restores `m_deviceAddress` from the USB port.
- **`qrc:/` firmware path not openable.** `QFile` does not understand the
  `qrc:/` URL syntax used by QML; the path is now normalised to the `:/`
  resource syntax before extracting the HEX to a temporary file (the temp file
  is kept alive for the duration of the upload).

### Changed
- `scripts/sync_firmware_from_zowiLibs.sh` now matches the current zowiLibs
  layout (`code/base/` and `code/games/<name>/<name>.hex`) and copies all game
  firmware, including Adivinawi.

## [0.5.0] - 2026-07-17

### Added
- **Choose USB or Bluetooth in the GUI.** The desktop app is no longer locked
  to Bluetooth: a new *Connection* selector in **Settings** lets the user pick
  **Automatic**, **Bluetooth** or **USB cable**. The `BluetoothController` is
  now transport-agnostic and can drive either the Qt/BlueZ SPP backend or the
  serial/USB backend.
- **Automatic transport detection (hybrid).** In *Automatic* mode the app
  detects the best available connection at startup and preselects it, giving
  USB priority when a robot is found on a port. The chosen transport is shown
  and can be overridden at any time; unavailable options are disabled.
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

### Changed
- The "no Bluetooth" splash banner now only appears when **neither** a
  Bluetooth adapter **nor** a USB robot is available, and its message guides
  the user to plug in via USB or enable Bluetooth.
- Home-screen auto-connect on launch now connects over USB when USB is the
  active transport and a robot is present, falling back to the saved Bluetooth
  device otherwise.

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
  the system Qt 6, with automatic `CAP_NET_ADMIN` assignment via `postinst` so
  BlueZ SPP works without a manual `setcap`.

## [0.1.0] - 2026-07-13

### Added
- First release of Zowi Desktop.
- Graphical wizard to scan, pair and connect to a Zowi robot over Bluetooth
  (BlueZ SPP).
- Command-line tool (`zowi_cli`) to control, program and flash Zowi firmware.
- Multi-language user interface (Spanish, Catalan, English, French, Bulgarian)
  backed by a translation engine and JSON locale files.
- Settings and firmware-flash screens, plus a status bar reflecting connection
  states.
