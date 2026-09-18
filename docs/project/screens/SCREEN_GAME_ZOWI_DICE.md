# SCREEN_GAME_ZOWI_DICE — GameZowiDiceScreen.qml

> Game 02 — **"Zowi Dice"** (Memory / *Zowi dice*): Zowi plays a growing random
> sequence of 4 moves and the player must repeat it from memory.
> Logic ported from ZowiAppReborn's `ZowiSaysMinigamePresenterImpl`, adapted to
> the desktop architecture (Qt-free core + thin GUI adapter). Intended as a new
> game alongside [SCREEN_GAME_TIMELINE.md](SCREEN_GAME_TIMELINE.md) and
> [SCREEN_GAME_MOUTHS.md](SCREEN_GAME_MOUTHS.md); future games can be added.

- **Status:** ✅ **IMPLEMENTED** (core + GUI, v0.10.x M10).
- **File:** `src/views/screens/GameZowiDiceScreen.qml`.
- **Core logic (Qt-free):** `src/core/include/zowi/zowi_dice.h` +
  `src/core/src/zowi_dice.cpp` — pure state machine, unit-tested in
  `src/core/tests/test_zowi_dice.cpp`.
- **GUI adapter:** `src/gui/controllers/ZowiDiceController.{h,cpp}`, exposed to
  QML as the context property `ZowiDice`.
- **i18n context:** `"GameZowiDiceScreen.qml"`.
- **Game id:** `zowi_says` — Home tile
  `qrc:/images/android/simon_game_button.png` (now `enabled: true` in
  [SCREEN_HOME.md](SCREEN_HOME.md); visible label comes from the Home context
  key `zowi_says`, e.g. *"Memoria"* in es_ES).
- **Source of truth (Android):** `ZowiSaysMinigamePresenterImpl`
  (`addRandomCommandToZowiSequence` → one new random move per round; playback as
  `[move, stop, move, stop, …]` ACK-chained over `"A"`/final ACK).
- **Connection:** interactive screen. The 4 action buttons are conn-gated
  (`Robot.connected`) and blocked while Zowi replays
  (`ZowiDice.blockUserInput`).
- Reached from Home *Zowi dice* tile → `main.qml` `pushZowiDice()` → push;
  back → pop.

## Architecture

State machine lives in core (`zowi::ZowiDiceGame`), fully Qt-free:

| State | Meaning |
|-------|---------|
| `Idle` | Initial / after `reset()` |
| `ShowingSequence` | Zowi replays the growing sequence; input blocked |
| `WaitingForUser` | Zowi finished replaying; player repeats from memory |
| `GameOver` | Wrong input (or round exceeded); score = len − 1 |

Command strings are built by core (`robot_commands.h`) and sent by the
controller through `RobotController::sendData()`. ACK sequencing is driven by
`RobotController::finalAckReceived`.

## QML context used

- `ZowiDice` (the controller): `state`, `score`, `sequenceLength`, `progress`,
  `blockUserInput`, `connected`; invokables `startGame()`, `resetGame()`,
  `onActionTopLeft()`, `onActionTopRight()`, `onActionBottomLeft()`,
  `onActionBottomRight()`; signal `gameOver(int)`.
- `Robot`: `connected` (gates the 4 action buttons), `sendData()`,
  `setDataPollingEnabled()` (paused while the screen is open, like PadScreen),
  `finalAckReceived` (wired to the controller).
- `Config.get(...)`: theme colors.
- `Session` (via controller): persists the last score.
- `Translator` (via `tr()`).

`CommandsController` is injected into the controller constructor wiring but is
not used by the game logic — movement strings come from core
(`ZowiDiceGame::nextRobotCommand()`), matching the layer rules in AGENTS.md.

## Commands sent

Produced by core (`ZowiDiceGame::buildCommandForAction`), default
`initialSpeedMs = 1000` (`MovementSpeed::Medium`):

| Action | Builder | Wire (medium) |
|--------|---------|----------------|
| Top-left (Walk) | `commandWalkForward` | `M 1 1000\r` |
| Top-right (Bend) | `commandBendBackward` | `M 16 1000\r` |
| Bottom-left (Jump) | `commandJump` | `M 11 1000\r` |
| Bottom-right (Moonwalker) | `commandMoonwalkerRight` | `M 7 1000 30\r` |
| End of each move | `commandStop` | `S\r` |

Buttons use the Android ZowiSays assets already shipped in this repo:
`move1_button.png`/`move2_button.png`/`move3_button.png`/`move4_button.png`
(and `pressed_*` variants). The help dialog shows `simon_game_button.png`.

## Gameplay

- **Round:** on entering the screen the game auto-starts (`Component.onCompleted`
  → `ZowiDice.startGame()`). Zowi replays the random sequence (length 1, growing
  by 1 per correct repeat). Each action is delivered as `[move … ACK] → [stop …
  ACK]`; the core only advances to the next action after the **Stop** ACK
  (movement ACK alone does not advance), exactly like the Android timeline.
  While Zowi plays, the 4 buttons are disabled and a progress bar runs.
- **Human turn:** once Zowi finishes, the state becomes `WaitingForUser` and the
  player repeats the sequence with the 4 buttons.
- **Wrong or extra input** → `GameOver` with `score = sequence length − 1`.
- **Correct full repeat** → one random move is appended and a new
  `ShowingSequence` round starts (matching `checkCurrentUserSequece()` in
  Android). Score displayed via `ZowiDice.score` (`score_prefix`, e.g.
  *"Puntuación: %1"*).
- **Game over dialog** (`game_over`, `final_score` `%1`, OK/Retry): Retry →
  `startGame()`, OK → `resetGame()`. If `score ≥ 12` a *"New best"* toast line
  is shown (achievement `in_love` reserved — see below).

## UI layout

- 2×2 action grid centered in the content area (uses actual screen
  `contentArea`, not a `parent.contentArea` lookup).
- Footer: score text, sequence progress bar (visible only while
  `ShowingSequence`), and a control row with **Play** (Idle/GameOver), **Help**
  (Idle) and **Ranking** (Idle, *disabled placeholder*).
- Dialogs are standard `QtQuick.Controls.Dialog` (non-Android `MakerBoxDialog`)
  with explicit `width` to avoid implicit-size binding loops.
- Back button inherited from `ScreenTemplate` (`backClicked` — do **not**
  redeclare the signal in the screen).

## Persistence

- `zowi_says_last_score` — latest score, saved by the controller on game over
  via `SessionController`.

## i18n

Keys under `"GameZowiDiceScreen.qml"` with translations in all 5 locales (es,
en, fr, ca, bg): `title`, `subtitle`, `play_button`, `help_button`,
`ranking_button`, `score_prefix` (`%1`), `walk_forward`, `bend_backward`,
`jump`, `moonwalker_right`, `how_to_play_text`, `close`, `game_over`,
`final_score` (`%1`), `new_best`.

## Tests

Core logic is covered by `test_zowi_dice.cpp` (registered in
`src/core/tests/CMakeLists.txt`). Regression test 11 drives the exact
controller flow (`nextRobotCommand()` + `onFinalAck()`) for a 2-action round to
guarantee `[move, stop, move, stop]` playback.

## Known deviations from the Android original (not yet implemented)

- **Ranking:** no leaderboard. The Ranking button is a disabled placeholder;
  `rankThreshold` is defined in `ZowiDiceConfig` but unused.
- **Achievements:** `in_love` (score ≥ 12) is not enforced; the game-over
  dialog simply shows a *"New best"* line. `achievementThreshold` is reserved.
- **No `ANGRY` gesture** on game over (Android plays `H8`).
- **No first-play help overlay** — Help is a manual button.
- **Speed is fixed** (1000 ms) via `ZowiDiceConfig::initialSpeedMs`; not
  adjustable in the UI.