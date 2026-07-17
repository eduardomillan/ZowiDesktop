# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
