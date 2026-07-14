# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
