# SCREEN_GAME_MEMORY — GameMemoryScreen.qml

> Game 02 — **"Zowi Dice"** (Memory / *Zowi dice*): Zowi plays a growing random
> sequence of 4 moves and the player must repeat it from memory.
> The four moves are **tiptoe-swing**, **bend backward**, **jump** and
> **moonwalker right**.
> Logic ported from ZowiAppReborn's `ZowiSaysMinigamePresenterImpl`, adapted to
> the desktop architecture (Qt-free core + thin GUI adapter). Intended as a new
> game alongside [SCREEN_GAME_TIMELINE.md](SCREEN_GAME_TIMELINE.md) and
> [SCREEN_GAME_MOUTHS.md](SCREEN_GAME_MOUTHS.md); future games can be added.

- **Status:** ✅ **IMPLEMENTED** (core + GUI, v0.10.x M10).
- **File:** `src/views/screens/GameMemoryScreen.qml`.
- **Core logic (Qt-free):** `src/core/include/zowi/zowi_dice.h` +
  `src/core/src/zowi_dice.cpp` — pure state machine, unit-tested in
  `src/core/tests/test_zowi_dice.cpp`.
- **GUI adapter:** `src/gui/controllers/ZowiDiceController.{h,cpp}`, exposed to
  QML as the context property `ZowiDice`.
- **i18n context:** `"GameMemoryScreen.qml"`.
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
`RobotController::softwareAckReceived` (`&&A`) + `finalAckReceived` (`&&F`)
and implemented in core by a per-action machine built on the shared
`zowi::MovementSequencer` (the same component the CLI uses for its
`runMovementCycles`).

## QML context used

- `ZowiDice` (the controller): `state`, `score`, `sequenceLength`, `progress`,
  `currentStep`, `blockUserInput`, `connected`; named state constants
  `stateIdle`, `stateShowingSequence`, `stateWaitingForUser`, `stateGameOver`
  (exposed as value properties, since QML enum lookups do not resolve for
  context-property instances); invokables `startGame()`, `resetGame()`,
  `onActionTopLeft()`, `onActionTopRight()`, `onActionBottomLeft()`,
  `onActionBottomRight()`; signal `gameOver(int)`.
- `Robot`: `connected` (gates the 4 action buttons), `sendData()`,
  `setDataPollingEnabled()` (paused while the screen is open, like PadScreen),
  `softwareAckReceived` + `finalAckReceived` (both wired to the controller —
  the game reacts to the movement's `&&A` to queue the Stop, and to the
  Stop's `&&F` to advance).
- `Config.get(...)`: theme colors.
- `Session`: persists the last score (via controller) and the first-play help
  flag (`zowi_says_help_seen`, read/written from QML for the auto-help).
- `Translator` (via `tr()`).

`CommandsController` is injected into the controller constructor wiring but is
not used by the game logic — movement strings come from core
(`ZowiDiceGame::nextRobotCommand()`), matching the layer rules in AGENTS.md.

## Commands sent

Produced by core (`ZowiDiceGame::buildCommandForAction`), default
`initialSpeedMs = 1000` (`MovementSpeed::Medium`):

| Action | Builder | Wire (medium) |
|--------|---------|----------------|
| Top-left (Tiptoe swing) | `commandTiptoeSwing` | `M 14 1000 15\r` |
| Top-right (Bend) | `commandBendBackward` | `M 16 1000\r` |
| Bottom-left (Jump) | `commandJump` | `M 11 1000\r` |
| Bottom-right (Moonwalker) | `commandMoonwalkerRight` | `M 7 1000 30\r` |
| End of each move | `commandStop` | `S\r` |

Buttons use the Android assets already shipped in this repo: the top-left
(now tiptoe-swing) uses `swing_button.png` (and `pressed_swing_button.png`),
the other three use `move2_button.png`/`move3_button.png`/`move4_button.png`
(with their `pressed_*` variants). The help dialog shows `simon_game_button.png`.

**Verified against the original:** `ZowiAppReborn`'s `ZowiSaysMinigamePresenterImpl`
(`playButtonPressed()`) builds three of the four moves with the same command
strings: `BEND` + direction `RIGHT` (`M 16`), `JUMP` (`M 11`) and `MOONWALKER` +
direction `RIGHT` (`M 7 <dur> 30`) — byte-for-byte the wire strings above. The
**top-left move is a deliberate deviation**: the Android original plays
`WALK FORWARD` (`M 1`), and this desktop version uses **tiptoe-swing** (`M 14`)
instead. Note the Android naming is `BEND RIGHT` for `M 16`, which the firmware
calls *bend backward* (`zowi.bend(1,T,-1)`); both refer to the same MoveID, so
our `BendBackward` label is the same physical gesture.

## Gameplay

- **Round:** the game does **not** auto-start on entry — the state is `Idle` and
  a prominent **Play** button is shown so the user can start when they want
  (or go back). The **help dialog** opens on entry according to the config key
  `zowi_dice_help` in `config.json`: `"always"` opens it every time and
  `"once"` only the first time (`zowi_says_help_seen` flag in Session); in
  both cases dismissing it does **not** start the game. Zowi replays the random
  sequence (length 1, growing by 1 per correct repeat). Each action is
  delivered as `[move … &&A] → [stop] → [&&F(move)] → [&&A(stop)] → [&&F(stop)]`:
  the Stop is queued as soon as the move's software ack (`&&A`) arrives, so it
  lands **mid-move**; only the Stop's final ack advances to the next action.
  This is the `MovementSequencer` protocol the CLI uses: the firmware repeats
  the last movement for one gait cycle per loop pass and reads serial only
  between cycles, so a Stop sent after the move's final ack would let an extra
  cycle slip in (the movement would visibly run twice). The game therefore
  waits for the move's `&&A` to queue the Stop, and — like the CLI — drains the
  Stop's own `&&A`/`&&F` before sending the next move, so stale acks never leak
  into the next action. While Zowi plays, the 4 buttons are
  disabled, a full-screen **"Look at Zowi"** overlay shows an animated robot +
  `look_at_zowi_text`, and a progress bar runs.
- **Human turn:** once Zowi finishes, the state becomes `WaitingForUser` and the
  player repeats the sequence with the 4 buttons. The progress bar stays
  visible and shows an **"X / Y" readout** of moves repeated so far
  (`currentStep`), like `setProgressValue()` in Android.
- **Wrong or extra input** → `GameOver` with `score = sequence length − 1`.
  The controller sends an **ANGRY gesture** (`H 8`) to the robot
  (`sendGameOverGesture()`), mirroring the Android game-over animation.
- **Correct full repeat** → one random move is appended and a new
  `ShowingSequence` round starts (matching `checkCurrentUserSequece()` in
  Android). Score displayed via `ZowiDice.score` (`score_prefix`, e.g.
  *"Puntuación: %1"*).
- **Game over dialog** (`game_over`, `final_score` `%1`, *"New best"* line for
  `score ≥ 12`): custom content like the help dialog — bold title, big final
  score, a **"Cerrar"** (outlined, → `resetGame()`) and a **"Reintentar"**
  accent pill (→ `startGame()`) button. The achievement `in_love` stays
  reserved — see below.

## UI layout

- The 2×2 action grid sits inside a rounded **"maker box" card** (accent border
  + app-tinted background), centered in the content area (uses actual screen
  `contentArea`, not a `parent.contentArea` lookup).
- **"Look at Zowi" overlay:** semi-transparent full-screen layer
  (`lookAtZowiOverlay`, visible while `ZowiDice.blockUserInput`) with an
  `AnimatedZowi` sprite and the `look_at_zowi_text` message, mirroring Android's
  `blockUserControls`/`showUserControls`.
- Footer (inside `ScreenTemplate`'s real footer area, so it **never overlaps
  the board**): the progress bar with **"X / Y"** readout (visible during
  `ShowingSequence` **and** `WaitingForUser`) plus, at the **bottom of the
  window**, a single prominent **Play** button (Idle/GameOver), styled like the
  splash *"Continuar"* button (200×50, bold 18 px, accent pill). The score does
  **not** live in the footer anymore.
- **Score strip:** the score sits in the content area, horizontally centered in
  the band between the board and the footer (`score_prefix`, bottom-anchored
  with a 6 px margin). The board sizes itself to
  `Math.min(parent.width, parent.height - 56) * 0.9`, reserving ~56 px at the
  bottom of the content area so the score can never overlap the 2×2 grid at any
  window size.
- **Help** ("Cómo jugar") and **Ranking** are **icon buttons top-right** (88×88,
  same slot/pattern as the achievements button on other screens), mirroring
  `activity_zowi_says_minigame_view.xml` where `minigame_help_button` +
  `minigame_ranking_button` sit top-right. They are assigned to the new
  `corner` slot of `ScreenTemplate` (a Row below the StatusBar, outside the
  clipped `contentArea`; hidden when no screen uses it). The two sit **adjacent**
  (`spacing: -10` in the corner Row, slightly overlapping): **Ranking first,
  Help immediately to its right**, so the pair stays close together like the
  Android original (tweak helps if it ever needs to widen/shrink). Ranking is
  a disabled placeholder.
  The disconnect button is **not** shown on this screen (back stays top-left;
  forget lives in Settings).
- Dialogs are standard `QtQuick.Controls.Dialog` (not the Android-native
  `MakerBoxDialog`), `modal` and centered on their parent with
  `anchors.centerIn` (QQC2 popups are not auto-centered).
- **Help dialog:** fully custom content (no default header/footer, so the
  rounded corners show). It shows a bold title (`help_button`), the
  `simon_game_button.png` image at 130 px, `how_to_play_text` (wrapped), and an
  accent pill **"Cerrar"** button (`close`, already translated in all 5
  locales) that closes the dialog. Sized `width: 460`, `radius: 20`, accent
  border. Its **height is content-driven** so it always fits the wrapped text
  and the Close button never sits on the border, whatever the locale text
  length. It opens directly in `Component.onCompleted` according to
  `zowi_dice_help`: `"always"` → every entry, `"once"` → only the first time
  (`zowi_says_help_seen`).
- **Game over dialog:** same custom treatment, `width: 360`, `radius: 20`,
  accent border, `anchors.centerIn: parent`, and the same content-driven height
  with 5% clearance. Content: `game_over` title, `final_score` (big, bold), the
  `new_best` line (when `score ≥ 12`), and a button row with an outlined
  **"Cerrar"** (→ `resetGame()`) and an accent pill **"Reintentar"**
  (`retry_button`, → `startGame()`).
- Back button inherited from `ScreenTemplate` (`backClicked` — do **not**
  redeclare the signal in the screen).

## Persistence

- `zowi_says_last_score` — latest score, saved by the controller on game over
  via `SessionController`.
- `zowi_says_help_seen` — set to `"true"` the first time the game screen loads;
  controls whether the help dialog auto-opens on entry.

## i18n

Keys under `"GameMemoryScreen.qml"` with translations in all 5 locales (es,
en, fr, ca, bg): `title`, `subtitle`, `look_at_zowi_text`, `play_button`,
`help_button`, `ranking_button`, `score_prefix` (`%1`), `how_to_play_text`,
`close`, `retry_button`, `game_over`, `final_score` (`%1`), `new_best`.

The move-name keys (`walk_forward`, `bend_backward`, `jump`,
`moonwalker_right` — legacy leftovers) also exist in the same context, but are
**not used by the QML**: the four action buttons are image-only, so move names
are never shown on screen.

## Tests

Core logic is covered by `test_zowi_dice.cpp` (registered in
`src/core/tests/CMakeLists.txt`). A shared `replayAction()` helper drives one
action through the full robot ACK chain (`M → &&A → S → &&F(move) → &&A(stop)
→ &&F(stop)`); Tests 3–12 use it for the game flow (rounds, scores, progress,
`currentStep`). Regression test 11 asserts the exact controller flow for a
2-action round, including that the move's `&&F` alone does **not** advance and
nothing is sent while the Stop is drained. Test 14 covers **stale acks** (a
`&&A`/`&&F` from a previous Stop arriving before the next move's `&&A` must be
ignored and must not skip or repeat a move). Test 13 asserts the tiptoe-swing
command is `M 14 1000 15\r` (MoveID 14, the swap for the Android's walk
forward `M 1`).

## Reliability

The controller arms a 20-second safety timer (same value as the CLI's
`MovementSequencer::startTimeoutMs()`) whenever a movement is sent. If the
move's `&&A` never arrives (e.g. the robot is busy or the transport dropped
it), the game would otherwise wait forever with the robot possibly still
moving: the timer stops the robot (`S`) and returns the game to `Idle` so the
player can retry.

## Known deviations from the Android original

- **Top-left move (implemented):** the Android original plays `WALK FORWARD`
  (`M 1`) as one of the four random moves; this desktop version swaps it for
  **tiptoe-swing** (`M 14`). Same gameplay, different move.
- **Ranking:** no leaderboard. The Ranking button is a disabled placeholder;
  `rankThreshold` is defined in `ZowiDiceConfig` but unused.
- **Achievements:** `in_love` (score ≥ 12) is not enforced; the game-over
  dialog simply shows a *"New best"* line. `achievementThreshold` is reserved.
- **Speed is fixed** (1000 ms) via `ZowiDiceConfig::initialSpeedMs`; not
  adjustable in the UI.
- **Help text does not mention the Zowi's name** (Android interpolates
  `%1$s` with the registered name).

## Planned work (week tracker)

Remaining items scheduled for implementation this week (see PLANNING.md M10).
Status is kept here until each item lands; implemented items move to the
relevant section (with a CHANGELOG entry) and are ticked off.

| Item | Status | Notes |
|------|--------|-------|
| **Ranking top-10** | 🚧 Planned | The top-right Ranking button is a **disabled placeholder** and there is **no `RankingController` in the repo yet**. Plan: a *shared* ranking layer (core top-10 + Session-backed persistence, keyed by game id) reused by Memory **and** Pintabocas (see `SCREEN_GAME_MOUTHS.md`). `rankThreshold = 3` is defined in `ZowiDiceConfig` but unused until then. |
| **Achievement `in_love`** (score ≥ 12) | ⏸ Deferred | Real achievement once the Achievements layer exists (future milestone). Today the game-over dialog only shows the *"New best"* line; `achievementThreshold` stays reserved. |
| **Adjustable speed** | 🚧 Planned | `ZowiDiceConfig::initialSpeedMs = 1000` is fixed; add a UI speed selector mapping to `MovementSpeed`. |
| **Help text with the Zowi's name** | 🚧 Planned | Android interpolates `%1$s` with the registered name; ours doesn't. Needs i18n work across the 5 locales. |
| **Dead i18n keys cleanup** | 🧹 Cleanup | `walk_forward`, `bend_backward`, `jump`, `moonwalker_right` under `"GameMemoryScreen.qml"` (5 locales) are unused (image-only buttons). Kept as documented legacy for now; remove during cleanup. |