# SCREEN_HOME — HomeScreen.qml

> Main dashboard: two swipeable pages — "Zowi Apps" (games/modes) and
> "Projects" (the Discover lessons). Top bar with the Settings button (left) and
> a **Ranking + Achievements** pair (right). Auto-reconnects to the robot on
> launch.

- **File:** `src/views/screens/HomeScreen.qml`
- **i18n context:** `"HomeScreen.qml"`
- Reached from Splash (`replace`) and after registration/wizard dismissal.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `settingsClicked()` | Settings button | `main.qml`: push→ Settings |
| `achievementsClicked()` | Achievements button | currently a stub (`console.log`); the button is **disabled and greyed out** until the achievements exist (M12) |
| `gamepadClicked()` | Gamepad app card | `main.qml`: push→ Pad |
| `zowiSaysClicked()` | Memory app card | `main.qml`: push→ Memory (`pushZowiDice`) |
| `timelineClicked()` | Timeline app card | `main.qml`: push→ GameTimeline (`pushTimeline`) |
| `mouthsClicked()` | Pintabocas app card | `main.qml`: push→ GameMouths (`pushMouths`) |
| `mouthEditorClicked()` | Mouths editor app card | `main.qml`: push→ MouthEditor |
| `projectRequested(projectId)` | A project tile | `main.qml`: push→ ProjectScreen |
| `goSplash()` / `goWelcome()` | DEV nav buttons | `main.qml`: replace→ Splash/Welcome |

The **Ranking** button has no signal: it opens the shared `RankingDialog`
(`Ranking.enabled` hides the button when `ranking_enabled` is `false`), the same
dialog the three games open from their corner button.

## QML context used

- `Ranking` (`enabled`) and `RankingDialog` (`showList()`).
- `Robot`: `connected`, `appId`, `battery`, `situation`,
  `SituationTransportLost`, `activeTransport`, `TransportUsb`,
  `usbAvailable`, `connectToDevice(addr)`, `connectUsb()`.
- `Session`: `loadActiveZowiDeviceAddress()`.
- `Config.get(...)`: theme colors; `Config.devMode`, `Config.devOverlayVisible`
  (DEV nav row).
- `Translator` (via `tr()`).

## Commands sent

- None directly. Auto-connect navigates the backend via `Robot.connectUsb()` or
  `Robot.connectToDevice(savedAddress)`.

## Implementation notes

- Auto-connect on `Component.onCompleted`: USB wins when it is the registered
  transport and available; otherwise reconnect to the saved Bluetooth address.
- The apps model enables `gamepad`, `timeline` (`SCREEN_GAME_TIMELINE.md`),
  `mouths_editor`, `zowi_says` (Memory game — `SCREEN_GAME_MEMORY.md`) and `mouths`
  (Pintabocas game — `SCREEN_GAME_MOUTHS.md`).
- **Corner buttons:** Ranking and Achievements sit in a `Row` anchored like the
  games' corner slot (below the status bar, 12 px from the right edge,
  `spacing: -10`). The ranking art is a half pill (flat right edge) and
  `achievements_d_button.png` is a "D" (flat left edge), so the pair reads as one
  pill, like ranking + help in the games.
- App buttons are gated by `home.robotReady`
  (`Robot.connected && appId !== "" && battery >= 0`); disabled cards are
  grayed out.
- DEV navigation row (Splash / Welcome) visible only with
  `Config.devMode && Config.devOverlayVisible`.

## Editable layout properties

Tuning knobs exposed on the screen root (adjust proportions at build/design
time without touching structure):

| Property | Default | Purpose |
|----------|---------|---------|
| `headerFontSize` | `22` | Pixel size of the "Zowi Apps \| Projects" header text |
| `appsWidthPercent` | `0.9` | Width of the Apps page container as a fraction of the window |
| `appsHeightPercent` | `1.0` | Height of the Apps page container as a fraction of the window |
| `projectsWidthPercent` | `0.9` | Width of the Projects page container as a fraction of the window |
| `projectsHeightPercent` | `1.0` | Height of the Projects page container as a fraction of the window |

## Project grid ordering

The order in which project tiles appear is the positional order of the
hard-coded array `projectsData` inside `HomeScreen.qml` —
left-to-right, then top-to-bottom in a `Grid`. There is no dynamic
sorting; reorder the entries in that array to change the on-screen order. Each
entry carries `id`, translated `name`, `icon` and an `enabled` flag (the ids
should match `projects/index.json`).

### Layout of the project tiles

Like the games row, the projects are spread **evenly over the whole width**: a
`Grid` with equal-width columns, each tile centred in its column. The number of
columns is what fits with `cellSpacing` (60 px) between the icons
(`floor((width + cellSpacing) / (iconSize + 8 + cellSpacing))`), so the distance
between icons stays about that value and only the leftover width is shared out.
Rows are separated by `cellSpacing`; the last row is left-aligned in the same columns.
