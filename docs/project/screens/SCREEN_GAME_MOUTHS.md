# SCREEN_GAME_MOUTHS — GameMouthsScreen.qml

> Game 03 — **"Pintabocas"** (Draw the mouth / *Bocas*): Zowi shows a random
> mouth and the player must draw it on a 6×5 LED grid before the countdown
> expires. Design derived from ZowiAppReborn's `MouthsMinigameActivity`
> (`MouthsMinigamePresenterImpl` + `activity_mouths_minigame_view.xml`).
> Intended as a new game alongside
> [SCREEN_GAME_TIMELINE.md](SCREEN_GAME_TIMELINE.md) and
> [SCREEN_GAME_ZOWI_DICE.md](SCREEN_GAME_ZOWI_DICE.md); future games can be added.

- **Status:** ⚠️ **NOT IMPLEMENTED** — design reviewed; implementation planned
  (see [Implementation plan](#implementation-plan-week)).
- **File:** `src/views/screens/GameMouthsScreen.qml` (does not exist yet).
- **i18n context:** `"GameMouthsScreen.qml"` (planned).
- **Game id:** `mouths` — Home tile `qrc:/images/android/mouths_game_button.png`
  (currently `enabled: false` in [SCREEN_HOME.md](SCREEN_HOME.md)). Distinct
  from the **Mouth Editor** tile (`mouths_editor`, already enabled), which owns
  the editor screen [SCREEN_MOUTH_EDITOR.md](SCREEN_MOUTH_EDITOR.md).
- **Source of truth (Android):** GAME_ID `MOUTHS_GAME_ID`;
  `MouthsMinigameActivity` / `MouthsMinigamePresenterImpl`,
  `VIEWS.md` §MouthsMinigameActivity; ranking under `MOUTHS_GAME_ID`.
- **Achievement:** `mouths_editor` — **score ≥ 8** at the end of a round; in
  Android this is the unlock for the Mouth Editor tile itself (see
  SCREEN_MOUTH_EDITOR.md). Reserved for the deferred ACHIEVEMENTS layer.
- **Connection:** **fully playable offline** on Android (local binary compare;
  Zowi show is cosmetic). The Play/round mechanics are not conn-gated; Zowi
  mouth + VICTORY/ANGRY gestures just don't appear if not connected.
- Reached from Home *Play* tile → push; back → pop.

## Signals (planned)

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `mouthsClicked()` | Home *Play* tile | `main.qml`: push→ `GameMouthsScreen.qml` |
| `playClicked()` | Play button (hidden after press) | start level 1 + countdown |
| `cellToggled(x, y)` | 6×5 grid (only touchable while round active) | redraw local matrix |
| `roundSolved()` / `timeout()` | round state machine | next level / end-of-game |
| `backClicked()` | inherited from `ScreenTemplate` | `main.qml`: `stack.pop()` |

## QML context used (planned)

- `Robot`: `sendData(cmd)` — optional cosmetics only (mouth + VICTORY/ANGRY);
  `connected` not required for game logic.
- `Commands`: mouth-by-matrix, VICTORY / ANGRY gestures, Stop.
- `Config.get(...)`: theme colors; **achievements enabled/disabled flag**.
- `Session`: last score / ranking gate (`mouths_last_score`).
- `Translator` (via `tr()`).

## Commands sent (planned)

| When | Builder | Wire (medium) |
|------|---------|----------------|
| Level start (Zowi shows the target mouth) | `Commands.mouth(matrix)` | `L 00<matrix>\r` |
| Correct draw | `Commands.gestureById(GestureVictory)` | `K <victory>\r` |
| Round end | `Commands.stop()` | `S\r` |
| Timeout / final fail | `Commands.gestureById(GestureAngry)` | `K <angry>\r` |

## Gameplay (planned)

- **Round:** one random mouth pattern (chosen from the 4 built-in matrices)
  is shown; Zowi displays it while a **10 s countdown** runs. The countdown
  shortens by 1 s every 4th level (L1–3: 10 s, L4–7: 9 s, …).
- **Player:** draws the mouth on the same 6×5 grid used by the editor
  ([SCREEN_MOUTH_EDITOR.md](SCREEN_MOUTH_EDITOR.md)); the grid is only
  touchable while a round is active. Submission compares the 30-bit string
  against the target matrix.
- **Correct** → VICTORY + Stop → next level. **Timeout** → end of game.
- **End of game:** `score = level − 1`. If `score ≥ 8` → `mouths_editor`
  achievement (reserved), else ANGRY. Ranking dialog if `score > 2` and within
  top-10 (`mouths` leaderboard). First play → help overlay.
- **Offline note:** game logic never requires Zowi; mouth/gesture commands are
  simply no-ops when not connected.

## Persistence (planned)

- `mouths_last_score` — latest score (for the ranking gate / toasts).
- `mouths_help_seen` — first-play help flag (mirrors `zowi_says_help_seen`),
  driven by the `mouths_help` config key (`"always"` / `"once"`).
- Ranking entries under game id `mouths` (in-Session top-10, mirroring
  `RankingController.saveRankingEntry` — shared layer planned with Zowi Dice).
  Cleared by "Forget playing history".

## i18n (planned)

New keys under `"GameMouthsScreen.qml"`: `title`, `play_button`, `help`,
`ranking_button`, `final_score`, `you_drew`, current game id already translated
on the Home context (`"mouths": "Bocas"`).

## Decisions taken (design review)

- **4 built-in matrices:** taken from the core `MouthId` presets
  (`src/core/include/zowi/robot_commands.h`, patterns in
  `zowi::kMouthPatterns`) — tentative set: **Smile, HappyOpen, Heart,
  TongueOut**. Trivially changeable in the game's config.
- **Countdown:** a **timer bar** (styled like the Zowi Dice progress bar),
  10 s on levels 1–3, −1 s every 4th level thereafter.
- **Target mouth while drawing:** shown **both on Zowi (if connected) and as an
  on-screen miniature** for the whole round — intentional desktop deviation
  from Android (which shows it once at level start), friendlier without a
  hardware round-trip.
- **Ranking:** a **shared layer** with Zowi Dice — one ranking module (core
  top-10 + Session persistence, keyed by game id) planned; until it exists the
  Ranking button stays a disabled placeholder (see `SCREEN_GAME_ZOWI_DICE.md`
  week tracker).

## Implementation plan (week)

Mirrors the Zowi Dice layering (Qt-free core + thin GUI adapter, per AGENTS.md):

| Piece | File | Content |
|-------|------|---------|
| Core game (Qt-free) | `src/core/include/zowi/mouths_game.h` + `src/core/src/mouths_game.cpp` | `MouthsGame` state machine: Idle / RoundActive / GameOver; round = target matrix (from `MouthId` presets) + 30-bit draw compare; `score = level − 1`; `score ≥ 8` → `mouths_editor` achievement (reserved); no Qt. |
| Core tests | `src/core/tests/test_mouths_game.cpp` (register in CMakeLists) | Matrix compare, level progression, timeout, score thresholds. |
| GUI adapter | `src/gui/controllers/MouthsGameController.{h,cpp}` → `MouthsGame` | `state`/`level`/`score`/`matrix`, `startGame()`, `onCellToggled(x,y)`, `submitDraw()`; QTimer countdown (GUI thread); commands `L 00<30bits>\r` (target), `K victory`, `K angry`, `S`; pauses data polling while open. |
| Shared grid | `src/views/components/MouthGrid.qml` | 6×5 LED grid extracted from `MouthEditorScreen.qml` (single `MouseArea` press/drag with `preventStealing`), reused by the game. |
| Screen | `src/views/screens/GameMouthsScreen.qml` (i18n context `"GameMouthsScreen.qml"`) | Grid, countdown bar, score, Play button, help overlay, game-over dialog, Ranking button. Playable offline (commands are no-ops when `!Robot.connected`). |
| Navigation | `main.qml` + `HomeScreen.qml` | `pushMouths()` (pattern `pushZowiDice`); enable Home tile `mouths` (`enabled: true`). |
| Config / session | `src/config.json` + Session | `mouths_help` (`"once"`/`"always"`); `mouths_last_score`, `mouths_help_seen`. |
| i18n | 5 locale files | New keys under `"GameMouthsScreen.qml"` (`title`, `play_button`, `help`, `final_score`, `you_drew`, …); `"mouths": "Bocas"` already on the Home context. |

### Open during implementation

- Exact wording of the new i18n keys (5 locales).
- Whether the shared ranking lands in this game's PR or a separate one.