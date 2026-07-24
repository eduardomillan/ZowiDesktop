# AGENTS.md

Compact guidance for OpenCode sessions in this repo. Read `.github/copilot-instructions.md` for the full architecture and conventions — this file only captures what an agent is likely to miss.

## Repo shape
- C++ core + two consumers: `src/core/` (Qt-free business logic), `src/cli/` (`zowi_cli`), `src/gui/` (Qt/QML app). GUI is an adapter layer, not a second business-logic layer.
- `src/backends/bt_qt/` is the single Bluetooth backend shared by GUI and CLI.
- Core tests in `src/core/tests/` link only `Zowi::core` — keep core Qt-free.

## Build & test (non-obvious)
- Normal build: `./build.sh` (Linux, Qt 6, GUI+CLI). Scoped: `./build.sh --gui`, `./build.sh --cli`, `./build.sh -5 --cli` (Qt 5), `./build.sh --demo`.
- Windows build: `build.bat` (GUI+CLI) from a **x64 Native Tools Command Prompt for VS 2022** (or any VS 2022 prompt that has `vcvarsall.bat` on PATH). Scoped: `build.bat --gui`, `build.bat --cli`. `windeployqt --qmldir src\views` runs automatically after GUI build.
- `QT_PATH=~/Qt/6.5.2/gcc_64 ./build.sh` to point at a specific Qt install.
- Core-only build (no Qt, fast): `cmake -S . -B build -DZOWI_BUILD_GUI=OFF -DZOWI_BUILD_CLI=OFF && cmake --build build`.
- Tests: `ctest --test-dir build --output-on-failure`. Single test: `ctest --test-dir build -R '^test_translation_engine$' --output-on-failure`. Build one: `cmake --build build --target test_translation_engine`.
- No lint/format/typecheck config exists in the repo — do not invent `lint`/`format` commands.

## Toolchain quirks
- GUI debug builds load QML from `src/views/main.qml` on disk and hot-reload `src/views/`; non-debug builds load QML/assets from `resources.qrc` / `*.qrc`. Don't "fix" missing on-disk assets in release mode.
- CLI needs `cap_net_admin` to flash firmware without sudo: `sudo setcap cap_net_admin+ep build/src/cli/zowi_cli` — re-apply after every rebuild. (`scripts/grant_bluetooth_cap.sh`.)
- Firmware HEX files come from an external `zowiLibs` repo, not this one. Sync with `scripts/sync_firmware_from_zowiLibs.sh [$ZOWILIBS_PATH]` (defaults to `~/zowiLibs`). It also normalises CRLF→LF for clean diffs.
- Translations: custom `TranslationEngine` reads `i18n/zowi_<locale>.ts`. Supported locales are hard-coded in `TranslationEngine::availableLocales()` — adding a locale means editing both the file set and that method. QML lookups depend on screen-context names like `Translator.translate("ScanScreen.qml", src)`; preserve those context names.
- Config is read from two places that must stay in sync: GUI via Qt resource `:/src/config.json`, CLI via workspace file `src/config.json`. Shared session keys: `activeZowiDeviceAddress`, `activeZowiName`, `activeZowiAppId`, `activeZowiBattery`, `activeZowiTransport`, `wizardDismissed`.
- Transport is not user-selectable: `RobotController` runs a situation state machine (`situation` enum: Demo/Unregistered/Connecting/Connected/Disconnected/TransportLost) derived from availability + registration + connection. The registered transport is tied to `activeZowiTransport` (bt/usb); changing it requires *forgetting* the Zowi. SettingsScreen shows status + contextual actions, not a transport picker. Design notes in `.local/transport_thoughts.md`.
- Runtime logs: `qDebug`/`qInfo`/`qWarning`/`qCritical` are mirrored to stderr and appended to a per-day file `ZowiDesktop-YYYY-MM-DD.log` under `QStandardPaths::AppDataLocation` (e.g. `~/.local/share/ZowiDesktop/` on Linux). One file per day; append mode keeps history across runs. Install happens early in `main.cpp` via `qInstallMessageHandler`.

## Website & releases (easy to break)
- The project website lives on the `gh-pages` branch under `docs/` and coexists with the signed apt repo (`docs/dists`, `docs/pool`, `docs/keyring.gpg`, `.nojekyll`). It is NOT in `main` (only `docs/project`, `docs/tests`, `docs/firmware` dev docs remain there).
- `release.yml` publishes the apt repo on `gh-pages` and is configured with `force_orphan: false` + `keep_files: true` precisely so the website survives. Do NOT flip `force_orphan` back to `true` — it would wipe the website.
- Releases are automatic from `v*` tags (AppImage + `.deb` jammy/noble + apt repo). Do not add a manual release step unless asked.
- The website is served at `https://eduardomillan.github.io/ZowiDesktop/docs/`; install docs point users at the apt repo there.
