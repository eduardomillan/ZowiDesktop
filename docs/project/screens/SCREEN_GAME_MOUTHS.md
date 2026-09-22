# SCREEN_GAME_MOUTHS — GameMouthsScreen.qml

> Game 03 — **"Pintabocas"** (Draw the mouths): Zowi shows a random
> mouth and the player must draw it on a 6×5 LED grid before the countdown
> expires. Design derived from ZowiAppReborn's `MouthsMinigameActivity`
> (`MouthsMinigamePresenterImpl` + `activity_mouths_minigame_view.xml`).
> Implemented alongside [SCREEN_GAME_MEMORY.md](SCREEN_GAME_MEMORY.md);
> future games can be added.

- **Status:** ✅ **IMPLEMENTED** (core + GUI).
- **File:** `src/views/screens/GameMouthsScreen.qml`.
- **Core logic (Qt-free):** `src/core/include/zowi/mouths_game.h` +
  `src/core/src/mouths_game.cpp` — pure state machine, unit-tested in
  `src/core/tests/test_mouths_game.cpp`.
- **GUI adapter:** `src/gui/controllers/MouthsGameController.{h,cpp}`, exposed to
  QML as the context property `Mouths`.
- **Drawing grid:** the shared `src/views/components/MouthGrid.qml` component,
  also used by the Mouth Editor ([SCREEN_MOUTH_EDITOR.md](SCREEN_MOUTH_EDITOR.md)).
- **i18n context:** `"GameMouthsScreen.qml"`.
- **Game id:** `mouths` — Home tile `qrc:/images/android/mouths_game_button.png`
  (`enabled: true` in [SCREEN_HOME.md](SCREEN_HOME.md)). Distinct
  from the **Mouth Editor** tile (`mouths_editor`, already enabled), which owns
  the editor screen [SCREEN_MOUTH_EDITOR.md](SCREEN_MOUTH_EDITOR.md).
- **Source of truth (Android):** GAME_ID `MOUTHS_GAME_ID`;
  `MouthsMinigameActivity` / `MouthsMinigamePresenterImpl`,
  `VIEWS.md` §MouthsMinigameActivity; ranking under `MOUTHS_GAME_ID`.
- **Achievement:** `mouths_editor` — **level ≥ 8** at the end of a round; in
  Android this is the unlock for the Mouth Editor tile itself (see
  SCREEN_MOUTH_EDITOR.md). Reserved for the deferred ACHIEVEMENTS layer
  (`achievementLevelThreshold = 8` in `MouthsGameConfig`).
- **Connection:** **fully playable offline** (local pattern compare;
  Zowi show is cosmetic). The Play/round mechanics are not conn-gated; Zowi
  mouth + VICTORY/ANGRY gestures just don't appear if not connected.
- Reached from Home *Pintabocas* tile → `main.qml` `pushMouths()` → push;
  back → pop.

## Architecture

State machine lives in core (`zowi::MouthsGame`), fully Qt-free:

| State | Meaning |
|-------|---------|
| `Idle` | Initial / after `reset()` |
| `RoundActive` | A target mouth is shown, player may draw |
| `RoundSolved` | Draw matched; victory intermission before next round |
| `GameOver` | Countdown expired; score = level − 1 |

Difficulty is a 5-band progressive pool (20 mouths, easiest to hardest):

| Band | Unlock level | Mouths |
|------|--------------|--------|
| band0 | 1 | Smile, Ok, SmallSurprise, HappyClosed |
| band1 | 3 | LineMouth, HappyOpen, Sad, Confused |
| band2 | 5 | SadOpen, SadClosed, Angry, Thunder |
| band3 | 8 | Heart, BigSurprise, TongueOut, Interrogation |
| band4 | 11 | X, Vamp1, Culito, Diagonal |

The round target comes only from the band unlocking at the current level, so
difficulty never regresses. No two consecutive rounds show the same mouth, and
no two consecutive games start with the same mouth (single-mouth bands fall
back to a repeat when exclusion is impossible).

Countdown: 10 s on levels 1–3, then −1 s every 4th level, floored at 2 s
(`initialCountdownMs`, `countdownStepMs`, `countdownStepEveryLevels`,
`minCountdownMs` in `MouthsGameConfig`).

## QML context used

- `Mouths` (the controller): `state`, `level`, `score`, `target` (MouthId for
  the miniature), `targetPattern` (32-bit pattern for the live compare),
  `countdownMs` / `roundTimeMs` (the bar works on countdown/round); named state
  constants `stateIdle`, `stateRoundActive`, `stateRoundSolved`,
  `stateGameOver` (exposed as value properties, since QML enum lookups do not
  resolve for context-property instances); invokables `startGame()`,
  `resetGame()`, `submitDraw(matrix)` (returns true when the draw just solved
  the round); signals `gameOver(score)`, `sendCommand(data)`.
- `Robot`: `connected` (drives the target-miniature policy), `sendData()`
  (cosmetics only, forwarded from `sendCommand` by `main.cpp` gated by
  connection state), `setDataPollingEnabled()` (paused while the screen is
  open, like the Mouth Editor).
- `Config.get(...)`: theme colors; `mouths_help` (`"always"` / `"once"`);
  `mouths_target_onscreen` (`"auto"` / `"always"` / `"never"`).
- `Session`: persists the last score (`mouths_last_score`, saved by the
  controller on game over) and the first-play help flag (`mouths_help_seen`,
  read/written from QML for the auto-help).
- `Translator` (via `tr()`).

`CommandsController` is not used directly — command strings come from core
(`commandMouth`, `commandGesture`, `commandStop` in `robot_commands.h`),
matching the layer rules in AGENTS.md.

## Commands sent

Produced by core, emitted as `sendCommand` and forwarded by `main.cpp` only
when connected:

| When | Builder | Wire (medium) |
|------|---------|----------------|
| Level start (Zowi shows the target mouth) | `commandMouth(targetPattern)` | `L 00<30 bits>\r` |
| Correct draw | `commandGesture(Victory)` + `commandStop()` | `K <victory>\r` then `S\r` |
| Timeout | `commandGesture(Angry)` + `commandStop()` | `K <angry>\r` then `S\r` |

The controller ticks the countdown every 100 ms and holds a 1.5 s VICTORY
intermission between a solved round and the next one.

## Gameplay

- **Round:** the game does **not** auto-start on entry — the state is `Idle` and
  a prominent **Play** button is shown so the user can start when they want
  (or go back). The **help dialog** opens on entry according to the config key
  `mouths_help`: `"always"` opens it every time and `"once"` only the first
  time (`mouths_help_seen` flag in Session); in both cases dismissing it does
  **not** start the game. Zowi shows the target mouth while the countdown bar
  runs.
- **Player:** draws the mouth on the shared 6×5 grid (same component as the
  editor); the grid is only touchable while a round is active. Every change is
  compared live against the target (mirrors Android's
  `MouthGridLayoutTouchListener` → `checkLedMouth`): a match solves the round.
- **Correct** → VICTORY + Stop → next level after the intermission (the grid
  clears on every new round). **Timeout** → end of game (the grid clears).
- **End of game:** `score = level − 1`, saved as `mouths_last_score`. The
  game-over dialog offers **"Cerrar"** (→ `resetGame()`, back to Idle with the
  Play button) and **"Reintentar"** (→ `startGame()`).
- **Offline note:** game logic never requires Zowi; mouth/gesture commands are
  simply no-ops when not connected.

## UI layout

- The drawing grid + target miniature sit inside a rounded **"maker box" card**
  (accent border + app-tinted background), centered in the content area. The
  card size derives from the content area first (responsive cell size), keeping
  clear of the level/countdown band on top and the score strip at the bottom,
  so the grid never covers the progress bar at any window size.
- **Target miniature:** mirrors the mouth shown on the robot. Policy driven by
  `mouths_target_onscreen`: `"auto"` (default) shows it only while the robot
  is NOT connected, `"always"` forces it, `"never"` hides it. The score stays
  centered under the drawing grid in both states.
- **Level + countdown bar** above the card (visible while a round runs or its
  victory intermission lasts), styled like the Memory progress bar.
- Footer (inside `ScreenTemplate`'s real footer area): a single prominent
  **Play** button (Idle/GameOver), styled like the splash *"Continuar"* button.
  The score lives in its own strip between the card and the footer
  (`score_prefix`, bottom-anchored).
- **Help** ("Cómo jugar") and **Ranking** are **icon buttons top-right**
  (same slot/pattern as the Memory game). Ranking is a disabled placeholder.
- Dialogs are standard `QtQuick.Controls.Dialog`, `modal` and centered, with
  custom content and content-driven height (same treatment as the Memory game).
- Back button inherited from `ScreenTemplate` (`backClicked` — do **not**
  redeclare the signal in the screen).

## Persistence

- `mouths_last_score` — latest score, saved by the controller on game over
  via `SessionController`.
- `mouths_help_seen` — set to `"true"` the first time the game screen loads
  (only under `mouths_help: "once"`); controls whether the help dialog
  auto-opens on entry.

## i18n

Keys under `"GameMouthsScreen.qml"` with translations in all 5 locales (es,
en, fr, ca, bg): `title`, `subtitle`, `play_button`, `help_button`,
`ranking_button`, `score_prefix` (`%1`), `level_prefix` (`%1`),
`target_label`, `how_to_play_text`, `close`, `game_over`, `final_score`
(`%1`), `retry_button`.

The Home tile label comes from the Home context key `mouths`
(*"Pintabocas"* in es_ES).

## Tests

Core logic is covered by `test_mouths_game.cpp` (registered in
`src/core/tests/CMakeLists.txt`): basic flow, wrong/correct draws,
level progression across bands, countdown formula (`L1/L3 → 10 s`, `L4 → 9 s`,
floor 2 s), timeout scoring (`score = level − 1`), reserved
achievement/ranking gates, reset, no consecutive repeats over 200 rounds, no
repeated first target over 50 games, and the single-mouth-band fallback.

## Known deviations from the Android original

- **Difficulty pool (implemented):** the Android original picks from 23
  patterns across 4 types at random; this desktop version uses a **progressive
  5-band pool of 20 mouths** (one band per level gate), with no consecutive
  repeats.
- **Target miniature (implemented):** Android shows the target once at level
  start on the robot; this desktop version adds a configurable **on-screen
  miniature** (`mouths_target_onscreen`), hidden by default while connected.
- **Ranking:** no leaderboard. The Ranking button is a disabled placeholder;
  `rankScoreThreshold = 2` is defined in `MouthsGameConfig` but unused.
- **Achievements:** `mouths_editor` (level ≥ 8) is not enforced; the game-over
  dialog simply shows the final score. `achievementLevelThreshold` stays
  reserved.

## Planned work

Remaining items (see PLANNING.md M10 / Future milestones). Status is kept here
until each item lands.

| Item | Status | Notes |
|------|--------|-------|
| **Ranking top-10** | 🚧 Planned | The top-right Ranking button is a **disabled placeholder** and there is **no `RankingController` in the repo yet**. Plan: a *shared* ranking layer (core top-10 + Session-backed persistence, keyed by game id) reused by Memory **and** Pintabocas (see `SCREEN_GAME_MEMORY.md`). `rankScoreThreshold = 2` is defined in `MouthsGameConfig` but unused until then. |
| **Achievement `mouths_editor`** (level ≥ 8) | ⏸ Deferred | Real achievement once the Achievements layer exists (future milestone). `achievementLevelThreshold` stays reserved. |
