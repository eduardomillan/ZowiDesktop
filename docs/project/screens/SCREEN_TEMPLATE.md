# SCREEN_TEMPLATE — ScreenTemplate.qml

> Base layout shared by every screen (excluded from searches): optional
> `StatusBar`, centered title/subtitle header, optional back/disconnect corner
> buttons, a `content` area and an optional `footer` area.

- **File:** `src/views/screens/ScreenTemplate.qml`
- **i18n context:** `"ScreenTemplate.qml"` (only the "disconnect" button label)
- **Not a screen** — screens are `ScreenTemplate { ... }` subclasses.

## Signals

| Signal | Emitted by |
|--------|------------|
| `backClicked()` | the back button (visible with `showBackButton: true`) |
| `disconnectClicked()` | the disconnect button (visible with `showDisconnectButton: true`, only while `Robot.connected`) |

## Properties

- `title` / `subtitle` — set `tr()`-translated text.
- `showBackButton`, `showDisconnectButton` — corner buttons.
- `showStatusBar` (default `true`).
- `footerHeight` — reserves space for the `footer` alias (e.g. MessageBar or a
  button row).

## QML context used

- `Config.get(...)` theme colors: `color_bg_app`, `color_bg_hover`,
  `color_error`, `color_danger`, `color_primary`.
- `Robot.connected` — gates the disconnect button.
- `Translator.translate("ScreenTemplate.qml", source)` — via `tr()`, for the
  disconnect button label only.

## Implementation notes

- Declares `default property alias content` and `footer`, so subclasses just
  place their items as children.
- Back button uses `qrc:/images/android/back_button.png`; emits `backClicked()`.