# SCREEN_GAME_TIMELINE — GameTimelineScreen.qml

> Game 01 — **"1, 2, 3 ¡Acción!"** (Sequence / *Línea de tiempo*): build a
> multi-step routine from movements, animations and mouths, then play it back
> on Zowi. Design derived from ZowiAppReborn's `TimelineActivity`
> (`TimelinePresenterImpl` + `activity_timeline_view.xml`). Intended as a new
> game alongside [SCREEN_GAME_ZOWI_SAYS.md](SCREEN_GAME_ZOWI_SAYS.md) and
> [SCREEN_GAME_MOUTHS.md](SCREEN_GAME_MOUTHS.md); future games can be added.

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/GameTimelineScreen.qml` (does not exist yet).
- **i18n context:** `"GameTimelineScreen.qml"` (planned).
- **Game id:** `timeline` — Home tile `qrc:/images/android/timeline_button.png`
  (currently `enabled: false` in [SCREEN_HOME.md](SCREEN_HOME.md)).
- **Source of truth (Android):** GAME_ID `TIMELINE_GAME_ID`;
  `TimelineActivity` / `TimelinePresenterImpl`, `VIEWS.md` §TimelineActivity;
  ranking under `TIMELINE_GAME_ID`.
- **Achievement:** `anxious` — Score ≥ 15 **commands in the sequence** at play.
  Reserved for the deferred ACHIEVEMENTS layer, exactly like the projects.
- **Connection:** interactive screen. **Play is conn-gated** (ship *Stop*, the
  add buttons and the list itself ungated, as on Android). No battery check.
- Reached from Home *Play* tile → push; back → pop.

## Signals (planned)

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `timelineClicked()` | Home *Play* tile | `main.qml`: push→ `GameTimelineScreen.qml` |
| `mouthSelectorRequested()` | Add Mouth | push→ shared `MouthScreen.qml` selector (SCREEN_MOUTH.md) |
| `animationSelectorRequested()` | Add Animation | push→ shared `GestureScreen.qml` selector (SCREEN_GESTURE.md) |
| `movementSelectorRequested()` | Add Movement | push→ movement `GridCommand` selector (new) |
| `playClicked()` / `stopClicked()` | toolbar | sequence player start/cancel |
| `backClicked()` | inherited from `ScreenTemplate` | `main.qml`: `stack.pop()` |

## QML context used (planned)

- `Robot`: `connected` (gates Play), `sendData(cmd)`.
- `Commands`: builders for the sequence items (see table).
- `Config.get(...)`: theme colors; **achievements enabled/disabled flag**.
- `Session`: progress persistence (`timeline_sequence`).
- `Translator` (via `tr()`).

## Commands sent (planned)

| Item | Builder | Wire (medium) |
|------|---------|----------------|
| Movement (12) | `Commands.*` movement builders | `M <id> <T> [size]\r` |
| Animation (11) | `Commands.gestureById(id)` | `K <id+1>\r` |
| Mouth (20) | `Commands.mouthById(id)` | `L 00<matrix>\r` |
| Stop (appended at end) | `Commands.stop()` | `S\r` |

- The playlist itself is **not** a single wire command; it is played item by
  item through the existing core `MovementSequencer` (src/core), which times
  each command's duration and emits the next one.

## Gameplay (planned)

- **Drafting (ungated):** Add Movement / Animation / Mouth opens a picker,
  appends `TimelineCommand(cmd, repetitions=1)`. Movement & animation picks are
  achievement-gated on Android; mouth is not — mirror once ACHIEVEMENTS is on.
- **Editing:** per-item spinners — repetitions (only if repeatable), duration
  (only if the movement has allowed durations: slow/med/fast), direction (only
  if applicable). Long-press **drag reorder**; per-row **delete**.
- **Play (conn-gated):** waits for the `"A"` ack, expands each item by its
  repetitions, **appends a trailing `StopCommand`**, plays the whole sequence
  once (**no loop**), then stops. If the list contained **≥ 15 commands** at
  play → `anxious` achievement (reserved). First play → help overlay.
- **Stop:** cancels the sequence + sends `S\r`.

## Persistence (planned)

- `timeline_sequence` — the current playlist (JSON list of
  `{command, repetitions, duration, direction}`), saved on screen exit and
  restored on entry, mirroring `GameController.saveProgress(TIMELINE_GAME_ID)`
  / `loadProgress`. Cleared by "Forget playing history" (Settings).

## i18n (planned)

New keys under `"GameTimelineScreen.qml"`: `title`, `play_button`,
`stop_button`, `add_movement`, `add_animation`, `add_mouth`, `help`, current
game id already translated on the Home context (`"timeline": "Línea de tiempo"`).

## Open questions (for review)

- Drag reorder on desktop: reuse Android's approach (long-press drag) or a
  simpler up/down toothpick? Tentatively long-press drag.
- Playback timing: hard-code `period` per move (as on Android) vs reuse the
  Pad speed setting? Tentatively reuse the movement's own duration spinner.