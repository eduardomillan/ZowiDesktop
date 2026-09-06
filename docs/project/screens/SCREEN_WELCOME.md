# SCREEN_WELCOME — WelcomeScreen.qml

> Entry screen with two options: start the pairing wizard or visit the project
> website ("learn more"). Reached after pairing is forgotten, or first launch.

- **File:** `src/views/screens/WelcomeScreen.qml`
- **i18n context:** `"WelcomeScreen.qml"`

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `startWizard()` | Start button | `main.qml`: sets `Robot.setTransportPreference(Robot.TransportAuto)` and push→ Wizard |
| `knowMoreClicked()` | Learn-more button | `main.qml`: `Qt.openUrlExternally(Config.get("know_more") + "/" + locale)` |

## QML context used

- `Config.get(...)`: `start_image` and theme colors.
- `Translator` (via `tr()`).

## Commands sent

- None.

## Implementation notes

- Pure static landing page: logo + title + tagline + two buttons
  (filled accent "Start", outlined "Learn more"). No key handling.