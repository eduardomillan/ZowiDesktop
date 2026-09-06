# SCREEN_GAME_MOUTHS — GameMouthsScreen.qml

> Game 03 — **"Pintabocas"** (Draw the mouth / *Bocas*): Zowi shows a random
> mouth and the player must draw it on a 6×5 LED grid before the countdown
> expires. Design derived from ZowiAppReborn's `MouthsMinigameActivity`
> (`MouthsMinigamePresenterImpl` + `activity_mouths_minigame_view.xml`).
> Intended as a new game alongside
> [SCREEN_GAME_TIMELINE.md](SCREEN_GAME_TIMELINE.md) and
> [SCREEN_GAME_ZOWI_SAYS.md](SCREEN_GAME_ZOWI_SAYS.md); future games can be added.

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
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
- Ranking entries under game id `mouths` (in-Session top-10, mirroring
  `RankingController.saveRankingEntry`). Cleared by "Forget playing history".

## i18n (planned)

New keys under `"GameMouthsScreen.qml"`: `title`, `play_button`, `help`,
`ranking_button`, `final_score`, `you_drew`, current game id already translated
on the Home context (`"mouths": "Bocas"`).

## Open questions (for review)

- Which 4 built-in matrices to ship (Android hard-codes 4; confirm with the
  editor set or reuse `MouthId` presets).
- Countdown feel on desktop: timer bar vs numeric counter; tentatively a bar.
- Should the target mouth remain on Zowi while the player draws, or hide after
  a preview window (Android shows it once at level start)?