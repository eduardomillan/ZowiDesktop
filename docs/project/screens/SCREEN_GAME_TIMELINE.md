# SCREEN_GAME_TIMELINE — GameTimelineScreen.qml

> Game 01 — **"1, 2, 3 ¡Acción!"** (Sequence / *Línea de tiempo*): build a
> multi-step routine from movements, animations and mouths, then play it back
> on Zowi. Design derived from ZowiAppReborn's `TimelineActivity`
> (`TimelinePresenterImpl` + `activity_timeline_view.xml`). Intended as a new
> game alongside [SCREEN_GAME_MEMORY.md](SCREEN_GAME_MEMORY.md) and
> [SCREEN_GAME_MOUTHS.md](SCREEN_GAME_MOUTHS.md); future games can be added.

- **Status:** ✅ **IMPLEMENTED** (core + GUI) — timeline editor and visual
  playback with direction support for all directional movements (Crusaito,
  Bend, Shake Leg, Flapping), repetitions 1–5, slow/medium/fast for movements and
  mouths, correct outline highlight during repetitions, visual feedback
  (opacity dimming) during playback, and a score for the global ranking. The
  sequence is saved when leaving the screen and restored on entry (see
  *Persistence*).
- **File:** `src/views/screens/GameTimelineScreen.qml` (~780 lines) and the chip
  component `src/views/components/TimelineChip.qml`.
- **Core logic (Qt-free):** `src/core/include/zowi/timeline_command.h`
  (chips + JSON), `timeline_player.h` (step player paced by the robot's acks) and
  `timeline_score.h` (ranking score); tests `test_timeline_command`,
  `test_timeline_player`, `test_timeline_score`.
- **GUI adapter:** `src/gui/controllers/TimelineController.{h,cpp}`, context
  property `Timeline`.
- **i18n context:** `"GameTimelineScreen.qml"` (all 5 locales).
- **Game id:** `timeline` — Home tile `qrc:/images/android/timeline_button.png`
  (enabled and navigates to screen).
- **Source of truth (Android):** GAME_ID `TIMELINE_GAME_ID`;
  `TimelineActivity` / `TimelinePresenterImpl`, `VIEWS.md` §TimelineActivity;
  ranking under `TIMELINE_GAME_ID`.
- **Achievement:** `anxious` — Score ≥ 15 **commands in the sequence** at play.
  Reserved for the deferred ACHIEVEMENTS layer, exactly like the projects.
- **Connection:** interactive screen. **Play is conn-gated** (ship *Stop*, the
  add buttons and the list itself ungated, as on Android). No battery check.
- Reached from Home *Timeline* tile → `main.qml` `pushTimeline()` → push; back →
  saves the sequence and pops.

## Signals (implemented)

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `timelineClicked()` | Home *Timeline* tile | `main.qml`: push→ `GameTimelineScreen.qml` |
| `mouthSelectorRequested()` | Add Mouth | push→ shared `MouthScreen.qml` selector (SCREEN_MOUTH.md) |
| `animationSelectorRequested()` | Add Animation | push→ shared `GestureScreen.qml` selector (SCREEN_GESTURE.md) |
| `movementSelectorRequested()` | Add Movement | push→ movement `GridCommand` selector (new) |
| `playClicked(int count)` / `stopClicked()` | toolbar | `Timeline.play(items)` / `Timeline.stop()` |
| `backClicked()` | inherited from `ScreenTemplate` | `main.qml`: `stack.pop()` |

## QML context used (implemented)

- `Robot`: `connected` (gates Play), `sendData(cmd)`.
- `Timeline`: `play(items)`, `stop()`, `saveSequence(items)`, `loadSequence()`,
  `isPlaying`, `currentIndex` (playback state), `currentChipIndex` (visual chip
  highlight during playback), `commandSupportsDirection(name)`,
  `commandUsesFrontBackDirection(name)` (direction support queries for UI state);
  signal `sequenceCompleted(score, eligible)`.
- `Ranking` (`enabled`, `recordScore("timeline", score)`) and `RankingDialog`.
- `Config.get(...)`: theme colors, `timeline_help` (`"always"` / `"once"`), `message_duration`.
- `Session`: reads `activeZowiName` (it replaces `ZOWINAME` in the help text),
  `timeline_help_seen`.
- `Commands` is used by `TimelineController` (not by the QML) to build the wire strings.
- `Translator` (via `tr()`).

## Commands sent (implemented)

| Item | Builder | Wire (medium) |
|------|---------|----------------|
| Movement | `Commands.*` movement builders | `M <id> <T> [size]\r` |
| Animation | `Commands.gestureById(id)` | `H <id+1>\r` |
| Mouth | `Commands.mouthById(id)` | `L 00<matrix>\r` |
| Stop (on stop and after the last step) | `Commands.stop()` | `S\r` |

- **Playback** is driven by the core `zowi::TimelinePlayer`, paced by the robot's
  acks (not by fixed timers): every **repetition becomes its own step** (like
  Android), a movement advances on its `&&A`/`&&F`, and a gesture or mouth, once
  its `&&F` arrives, stays on for its duration (`m_motionlessDisplay`, slow 2 s /
  medium 1 s / fast 0.7 s) before the next step. A 20 s guard timer
  (`kMoveStartTimeoutMs`) stops playback if a movement's `&&A` never arrives. The
  controller also reports the current step/chip so the screen highlights it and
  auto-scrolls.

## Gameplay (GUI complete; backend deferred)

- **Drafting (ungated):** Add Movement / Animation / Mouth opens a picker dialog,
  appends new timeline command with defaults (reps=1, duration=Medium). ✅ Done.
- **Editing:** per-chip cycle buttons — repetitions **1–5** (movements and
  gestures; hidden for mouths, which ignore repetitions), duration slow/medium/fast
  (movements **and mouths**), direction (Crusaito, Bend, Shake Leg, Flapping).
  Long-press drag to reorder; per-chip delete button. Disabled during playback,
  with opacity fade for visual feedback. ✅ Done.
- **Play (conn-gated):** single button that swaps to Stop icon mid-playback.
  Sends commands to robot; highlights each chip as it plays with animated
  border; auto-scrolls to current chip; shows stop button with disabled-reason
  tooltips ("Connect Zowi", "Add a command"). ✅ Done (full playback implementation).
- **Clear:** one-click wipe now requires confirmation dialog. ✅ Done.
- **Help:** opens on entry according to `timeline_help` (`"always"`, as shipped in
  `config.json`, or `"once"` → first visit only, `timeline_help_seen`). It has a
  second paragraph, `how_to_play_ranking`, on how the sequence scores. ✅ Done.
- **Achievements:** `anxious` (15+ commands) reserved for achievements layer.

## Ranking (implemented)

Not in the Android original (Timeline had no ranking there); desktop addition.
Timeline adds to the **single global ranking** (see `RANKING_HOWTO.md`):

- Raw score = weighted complexity of the sequence (`timelineScore()` in core):
  movement 3 × reps, gesture 2 × reps, mouth 1; Fast +20 % / Slow −20 % on
  movements; +2 per distinct item type; first 40 chips only. In the ranking it
  is normalised (60 raw = 100 points) and the player's best counts.
- Awarded **only when the whole sequence ran to completion** with real robot
  acks. Stop, a timeout or a lost robot never score. At least 5 chips are needed.
- `TimelineController` emits `sequenceCompleted(score, eligible)`; the screen
  records it for the active player and opens the ranking when it improved. The
  corner Ranking button opens the list.

## Persistence (implemented)

- `timeline_sequence` — the current playlist (JSON list of the chips), written to
  the session by `Timeline.saveSequence()` when the user leaves the screen
  (`main.qml` `pushTimeline()`, back button) and read back by
  `Timeline.loadSequence()` from `Component.onCompleted`, before the help dialog
  decision. Mirrors Android's `GameController.saveProgress` / `loadProgress`
  (`TIMELINE_GAME_ID`). The "Clear" button empties the list; leaving the screen
  then saves the empty list.
- Android also cleared it with "Forget playing history" (Settings → delete
  achievements); see `SCREEN_SETTINGS.md`.

## i18n (implemented)

Complete key set under `"GameTimelineScreen.qml"` in all 5 locales (es_ES, en_US,
fr_FR, ca_ES, bg_BG): `title`, `subtitle`, `play_button`, `stop_button`,
`add_movement`, `add_animation`, `add_mouth`, `clear_timeline`, `close`, `help`,
`empty_timeline`, `how_to_play_text`, `how_to_play_ranking`, `repetitions`,
`duration`, `dir`, `play_disabled_no_robot`, `play_disabled_empty`,
`confirm_clear_title`, `confirm_clear_message`, `confirm_clear_action`, `cancel`,
`ranking_too_short` (a completed sequence under 5 chips does not count).

## Deferred

- **Per-rep progress:** each repetition is sent as its own step, but the screen
  highlights the whole chip; showing progress inside a multi-rep chip is not done.
- **Achievements:** `anxious` gating (≥15 commands) reserved for achievements layer.
