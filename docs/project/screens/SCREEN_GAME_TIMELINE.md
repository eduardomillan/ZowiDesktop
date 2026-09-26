# SCREEN_GAME_TIMELINE — GameTimelineScreen.qml

> Game 01 — **"1, 2, 3 ¡Acción!"** (Sequence / *Línea de tiempo*): build a
> multi-step routine from movements, animations and mouths, then play it back
> on Zowi. Design derived from ZowiAppReborn's `TimelineActivity`
> (`TimelinePresenterImpl` + `activity_timeline_view.xml`). Intended as a new
> game alongside [SCREEN_GAME_ZOWI_SAYS.md](SCREEN_GAME_ZOWI_SAYS.md) and
> [SCREEN_GAME_MOUTHS.md](SCREEN_GAME_MOUTHS.md); future games can be added.

- **Status:** ✅ **GUI IMPLEMENTED** (0.9.3–0.9.4) — timeline editor and visual
  playback (no backend sequencer yet). Backend persistence and `MovementSequencer`
  planned for backend phase.
- **File:** `src/views/screens/GameTimelineScreen.qml` (624 lines, complete).
- **i18n context:** `"GameTimelineScreen.qml"` (all 5 locales + 8 new keys in 0.9.4).
- **Game id:** `timeline` — Home tile `qrc:/images/android/timeline_button.png`
  (enabled and navigates to screen).
- **Source of truth (Android):** GAME_ID `TIMELINE_GAME_ID`;
  `TimelineActivity` / `TimelinePresenterImpl`, `VIEWS.md` §TimelineActivity;
  ranking under `TIMELINE_GAME_ID`.
- **Achievement:** `anxious` — Score ≥ 15 **commands in the sequence** at play.
  Reserved for the deferred ACHIEVEMENTS layer, exactly like the projects.
- **Connection:** interactive screen. **Play is conn-gated** (ship *Stop*, the
  add buttons and the list itself ungated, as on Android). No battery check.
- Reached from Home *Play* tile → push; back → pop.

## Signals (implemented)

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `timelineClicked()` | Home *Play* tile | `main.qml`: push→ `GameTimelineScreen.qml` |
| `mouthSelectorRequested()` | Add Mouth | push→ shared `MouthScreen.qml` selector (SCREEN_MOUTH.md) |
| `animationSelectorRequested()` | Add Animation | push→ shared `GestureScreen.qml` selector (SCREEN_GESTURE.md) |
| `movementSelectorRequested()` | Add Movement | push→ movement `GridCommand` selector (new) |
| `playClicked()` / `stopClicked()` | toolbar | sequence player start/cancel |
| `backClicked()` | inherited from `ScreenTemplate` | `main.qml`: `stack.pop()` |

## QML context used (implemented)

- `Robot`: `connected` (gates Play), `sendData(cmd)`.
- `Timeline`: `play()`, `stop()`, `isPlaying`, `currentIndex` (playback state).
- `Commands`: builders for the sequence items.
- `Config.get(...)`: theme colors, `timeline_help`/`memory_help` flags, locale.
- `Session`: read/store `activeZowiName` (for help text), `timeline_help_seen`.
- `Translator` (via `tr()`).

## Commands sent (implemented)

| Item | Builder | Wire (medium) |
|------|---------|----------------|
| Movement (12) | `Commands.*` movement builders | `M <id> <T> [size]\r` |
| Animation (11) | `Commands.gestureById(id)` | `K <id+1>\r` |
| Mouth (20) | `Commands.mouthById(id)` | `L 00<matrix>\r` |
| Stop (on stop) | `Commands.stop()` | `S\r` |

- The playlist is played item-by-item via `TimelineController::playNext()`, which
  uses a `QTimer` (`m_playbackTimer`) to sequence each command with the
  configured duration before emitting the next one. This provides the visual
  playback sequencing (highlight + auto-scroll) alongside robot command delivery.

## Gameplay (GUI complete; backend deferred)

- **Drafting (ungated):** Add Movement / Animation / Mouth opens a picker dialog,
  appends new timeline command with defaults (reps=1, duration=Medium). ✅ Done.
- **Editing:** per-chip cycle buttons — repetitions (movement only), duration
  (movement only, slow/medium/fast), direction (Crusaito only). Long-press drag
  to reorder; per-chip delete button. Disabled during playback. ✅ Done.
- **Play (conn-gated):** single button that swaps to Stop icon mid-playback.
  Sends commands to robot; highlights each chip as it plays with animated
  border; auto-scrolls to current chip; shows stop button with disabled-reason
  tooltips ("Connect Zowi", "Add a command"). ✅ Done (full playback implementation).
- **Clear:** one-click wipe now requires confirmation dialog. ✅ Done.
- **Help:** auto-opens on first visit (configurable `timeline_help` flag). ✅ Done.
- **Achievements:** `anxious` (15+ commands) reserved for achievements layer.

## Persistence (planned)

- `timeline_sequence` — the current playlist (JSON list of
  `{command, repetitions, duration, direction}`), saved on screen exit and
  restored on entry, mirroring `GameController.saveProgress(TIMELINE_GAME_ID)`
  / `loadProgress`. Cleared by "Forget playing history" (Settings).

## i18n (implemented)

Complete key set under `"GameTimelineScreen.qml"` in all 5 locales (es_ES, en_US,
fr_FR, ca_ES, bg_BG): `title`, `subtitle`, `play_button`, `stop_button`,
`add_movement`, `add_animation`, `add_mouth`, `clear_timeline`, `close`, `help`,
`empty_timeline`, `how_to_play_text`, `repetitions`, `duration`, `dir` (v0.9.3),
plus `play_disabled_no_robot`, `play_disabled_empty`, `confirm_clear_title`,
`confirm_clear_message`, `confirm_clear_action`, `cancel` (v0.9.4).

## Deferred (backend phase)

- **Persistence:** `timeline_sequence` JSON storage and load (SessionController).
- **Per-rep timing:** Playback fires all reps of a command back-to-back with no
  inter-rep delay; per-rep timing visualization (e.g., showing progress within a
  multi-rep move) deferred.
- **Achievements:** `anxious` gating (≥15 commands) reserved for achievements layer.
