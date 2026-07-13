# Copilot Instructions

## Build and test commands

- Use `./build.sh` for normal builds:
  - `./build.sh` builds both `ZowiDesktop` and `zowi_cli` against Qt 6 into `build/`
  - `./build.sh --gui` builds only the GUI executable
  - `./build.sh --cli` builds only the CLI executable
  - `./build.sh -5 --cli` builds the CLI against Qt 5
- Use direct CMake targets for scoped rebuilds:
  - `cmake -S . -B build -DZOWI_QT_VERSION=6 -DZOWI_BUILD_GUI=ON -DZOWI_BUILD_CLI=ON`
  - `cmake --build build --target ZowiDesktop`
  - `cmake --build build --target zowi_cli`
- Core tests do not require Qt and can be built in isolation:
  - `cmake -S . -B build -DZOWI_BUILD_GUI=OFF -DZOWI_BUILD_CLI=OFF`
  - `cmake --build build`
  - `ctest --test-dir build --output-on-failure`
  - Run a single test with `ctest --test-dir build -R '^test_translation_engine$' --output-on-failure`
  - Build a single test executable with `cmake --build build --target test_translation_engine`

## High-level architecture

- The repository is split into a pure C++ core plus two consumers:
  - `src/core/` contains business logic and data access with no Qt dependency
  - `src/cli/main.cpp` provides the `zowi_cli` interface for debugging and scripting
  - `src/gui/` provides the Qt/QML desktop app
- `src/backends/bt_qt/` is the Qt Bluetooth backend. Both the GUI and CLI use it rather than implementing Bluetooth separately.
- The GUI layer is an adapter layer, not a second business-logic layer. `src/gui/main.cpp` wires four context properties into QML: `Session`, `Translator`, `Bluetooth`, and `Config`. Those controllers wrap core/back-end classes and expose them to QML.
- In debug builds the GUI loads QML from `src/views/main.qml` on disk and watches `src/views/` for hot reload. In non-debug builds it loads QML and assets from `resources.qrc`.
- Core tests live in `src/core/tests/` and only link `Zowi::core`, so changes that can stay in `src/core/` should avoid pulling in Qt dependencies.

## Key conventions

- Keep `src/core/` Qt-free. New business logic belongs in core unless it genuinely depends on Qt or QML.
- Keep GUI and CLI behavior aligned when changing shared concerns:
  - session persistence via `SessionStore("ZowiDesktop", "ZowiApp")`
  - Bluetooth discovery/connection behavior
  - translation loading
  - config reads from `src/config.json`
- The GUI reads config from the Qt resource path `:/src/config.json`, while the CLI reads the workspace file `src/config.json`. If config keys or file layout change, both access paths must still work.
- Reuse the existing session keys instead of creating parallel names. Shared keys already used across the app include `activeZowiDeviceAddress`, `activeZowiName`, `activeZowiAppId`, `activeZowiBattery`, and `wizardDismissed`.
- Translations are handled by the custom `TranslationEngine`, which reads `i18n/zowi_<locale>.ts` files directly. Supported locales are hard-coded in `TranslationEngine::availableLocales()`, so adding a locale requires updating both the file set and that method.
- QML text is translated with screen-specific contexts such as `Translator.translate("ScanScreen.qml", source)`. Preserve those context names when moving strings between screens, because the translation lookup depends on them.
- Controllers in `src/gui/controllers/` are intentionally thin wrappers. Prefer exposing signals/properties from the controller over duplicating logic in QML.
- Bluetooth discovery uses the `zowi_mac_prefix` key from `src/config.json` in both the CLI and QML scan flow. If discovery filters change, update both surfaces together.
