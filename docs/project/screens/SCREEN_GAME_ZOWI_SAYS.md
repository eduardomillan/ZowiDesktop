# SCREEN_GAME_ZOWI_SAYS — GameZowiSaysScreen.qml

> Game 02 — **"Repite con Zowi"** (Memory / *Zowi dice*): Zowi plays a
> growing random sequence of 4 moves and the player must repeat it from memory.
> Design derived from ZowiAppReborn's `ZowiSaysMinigameActivity`
> (`ZowiSaysMinigamePresenterImpl` + `activity_zowi_says_minigame_view.xml`).
> Intended as a new game alongside [SCREEN_GAME_TIMELINE.md](SCREEN_GAME_TIMELINE.md)
> and [SCREEN_GAME_MOUTHS.md](SCREEN_GAME_MOUTHS.md); future games can be added.

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/GameZowiSaysScreen.qml` (does not exist yet).
- **i18n context:** `"GameZowiSaysScreen.qml"` (planned).
- **Game id:** `zowi_says` — Home tile
  `qrc:/images/android/simon_game_button.png` (currently `enabled: false` in
  [SCREEN_HOME.md](SCREEN_HOME.md)).
- **Source of truth (Android):** GAME_ID `ZOWI_SAYS_GAME_ID`;
  `ZowiSaysMinigameActivity` / `ZowiSaysMinigamePresenterImpl`,
  `VIEWS.md` §ZowiSaysMinigameActivity; ranking under `ZOWI_SAYS_GAME_ID`.
- **Achievement:** `in_love` — **score ≥ 12** at the end of a round.
  Reserved for the deferred ACHIEVEMENTS layer, exactly like the projects.
- **Connection:** interactive screen. All 4 action buttons are conn-gated
  while the round is running (Android enables them only while `isPlaying`).
  Play/Help/Ranking/Home are always enabled.
- Reached from Home *Play* tile → push; back → pop.

## Signals (planned)

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `zowiSaysClicked()` | Home *Play* tile | `main.qml`: push→ `GameZowiSaysScreen.qml` |
| `playClicked()` | Play button | start a new round (starts sequence of length 1) |
| `moveClicked(dir)` | one of the 4 action buttons | user repeats the sequence, one step at a time |
| `tick[duration]` / `gameOver(score)` | round state machine | progress bar / end-of-game dialog |
| `backClicked()` | inherited from `ScreenTemplate` | `main.qml`: `stack.pop()` |

## QML context used (planned)

- `Robot`: `connected` (gates the 4 action buttons), `sendData(cmd)`, ACK
  `"A"` used to know when Zowi finished replaying his turn.
- `Commands`: the 4 movements + stop (see table).
- `Config.get(...)`: theme colors; **achievements enabled/disabled flag**.
- `Session`: last score / ranking gate (`zowi_says_last_score`).
- `Translator` (via `tr()`).

## Commands sent (planned)

| Action | Builder | Wire (medium) |
|--------|---------|----------------|
| Top-left | `Commands.walkForward(speed)` | `M 1 <T>\r` |
| Top-right | `Commands.bendBackward(speed)` | `M 16 <T>\r` |
| Bottom-left | `Commands.jump(speed)` | `M 14 <T>\r` |
| Bottom-right | `Commands.moonwalkerRight(speed, 30)` | `M 7 <T> 30\r` |
| End of turn | `Commands.stop()` | `S\r` |

## Gameplay (planned)

- **Round:** Zowi plays a random sequence (starting at length 1, growing by 1
  per correct repeat) using the 4 moves; each move is delivered as
  `command + StopCommand`, and `"A"` ack marks the end of each step. While Zowi
  plays, the 4 buttons are blocked (mirrors Android's `blockUserControls`
  overlay) and a small progress bar shows the sequence being replayed.
- **Human turn:** once Zowi finishes, the player repeats the sequence. Each tap
  appends a command; **longer or wrong** sequence → end of game.
- **End of game:** `gameOver(score)` with `score = length reached − 1`. If
  `score ≥ 12` → `in_love` achievement (reserved), else `ANGRY` gesture
  (`Commands.gestureById(GestureAngry)`). Ranking dialog if
  `score > 3` and within top-10 (`zowi_says` leaderboard). First play → help
  overlay.
- **Turn-taking timing:** Zowi replays on his turn; wait for ACK before
  re-enabling the buttons (same handshake as Timeline).

## Persistence (planned)

- `zowi_says_last_score` — latest score (for the ranking gate / toasts).
- Ranking entries under game id `zowi_says` (in-Session top-10, mirroring
  `RankingController.saveRankingEntry`). Cleared by "Forget playing history".

## i18n (planned)

New keys under `"GameZowiSaysScreen.qml"`: `title`, `play_button`, `help`,
`ranking_button`, `final_score`, current game id already translated on the Home
context (`"zowi_says": "Zowi dice"`).

## Open questions (for review)

- Turn pacing: fixed per-move period (e.g. 1000 ms as Android) vs adjustable;
  tentatively fixed.
- Score display during the round (Android shows a progress bar only); propose
  showing current round length + best score.