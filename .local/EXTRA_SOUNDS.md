# Plan: Extra Sound Screens for ZowiDesktop

## Table of contents

- [Background (what already exists)](#background-what-already-exists)
- [Screen 1: MelodyScreen (melody player)](#screen-1-melodyscreen-melody-player)
- [Screen 2: PianoScreen (note player)](#screen-2-pianoscreen-note-player)
- [Shared implementation tasks (ordered)](#shared-implementation-tasks-ordered)
- [Out of scope / future ideas](#out-of-scope--future-ideas)

---

Two new screens built around Zowi's buzzer capabilities, mirroring how the
mouth features are exposed today:

1. **MelodyScreen** — a player for the 19 pre-programmed firmware melodies
   (`K <id>`), analogous to the mouth player (`MouthScreen`) reachable from
   the Gamepad.
2. **PianoScreen** — a piano keyboard that plays individual notes via the
   free-tone buzzer command (`T <freqHz> <ms>`), analogous to the Mouth
   Editor (a Home tile, "not really a game").

Status: design plan only — no code written yet.

## Background (what already exists)

- **Firmware protocol** (`docs/firmware/PROTOCOL.md`, `~/zowiLibs/code/base/ZOWI_BASE_v2.ino`):
  - `K <SingID>\r` → `receiveSing()` plays one of 19 melodies from
    `zowiLibs/arduinolibs/Zowi/Zowi_sounds.h`.
  - `T <freq> <ms>\r` → `recieveBuzzer()` plays a single tone
    (`zowi._tone(freq, duration, 1)`).
  - **Caveat for both**: the handler calls `sendAck()` then `zowi.home()`
    (servos recenter — visible side effect) and playback is *blocking*; the
    final ACK (`F`) only arrives when the tone/melody ends. Rapid taps on the
    piano therefore queue on the firmware side: keep tone durations short.
- **Core** (Qt-free, already tested):
  - `zowi::commandSing(MelodyId)` — `src/core/include/zowi/robot_commands.h:131`.
    `MelodyId` enum (0-based, 19 values) at line 123; wire order matches the
    firmware switch, kept in sync by `scripts/verify_arduino_mirrors.sh`.
  - `zowi::commandTone(freqHz, durationMs)` — `robot_commands.h:56`, covered by
    `src/core/tests/test_robot_commands.cpp:54-56`.
  - Note frequency table: `Zowi_sounds.h` defines `note_C0`…`note_B7`
    (e.g. `note_C4 = 261.63`, `note_A4 = 440`).
- **GUI adapter**:
  - `Commands.sing(int melodyId)` is already `Q_INVOKABLE`
    (`src/gui/controllers/CommandsController.cpp:140`) plus 19 `Melody*`
    constants — **but no QML screen uses it yet**.
  - `commandTone()` is **not exposed** in `CommandsController` — gap to fill.
- **UI conventions to follow**:
  - `MouthScreen.qml` (grid player entered from `PadScreen` via
    `mouthScreenRequested`, wired in `src/views/main.qml:30-33`).
  - `MouthEditorScreen.qml` (Home tile, wired in `main.qml:39-42`, registered
    in `HomeScreen.qml` apps list ~line 441 and `onClicked` switch ~line 234).
  - Screens are `ScreenTemplate` subclasses; i18n context = QML file name;
    release builds load from `views.qrc`, debug builds hot-reload from disk.

## Screen 1: MelodyScreen (melody player)

**File:** `src/views/screens/MelodyScreen.qml`

- `ScreenTemplate` with `screenName: "MelodyScreen"`, `showBackButton: true`,
  title `tr("melodies_title")`.
- Layout: `Flow` + `Repeater` over a JS array of the 19 melodies, in
  `MelodyId` enum order (Connection, Disconnection, Surprise, OhOoh, OhOoh2,
  Cuddly, Sleeping, Happy, SuperHappy, HappyShort, Sad, Confused, Fart1,
  Fart2, Fart3, Mode1, Mode2, Mode3, ButtonPushed).
- No per-melody image assets exist (unlike mouths). Two options:
  - **A (chosen):** styled text buttons (rounded `Rectangle` + translated
    label), like a simple chip grid — zero new assets.
  - B (later polish): add `melody_*_button.png` assets to
    `qrc:/images/android/` and switch to the `MouthScreen` image pattern.
- Tap → `Robot.sendData(Commands.sing(melodyId))` guarded by
  `Robot.connected`; highlight the selected chip; log to console like
  `MouthScreen.selectMouth()`.
- `Component.onCompleted: Robot.setDataPollingEnabled(false)` /
  restore on destruction (same as MouthScreen) so ACK/`F` traffic doesn't
  interfere.
- Because playback is blocking, disable/gray the grid while a melody plays if
  a duration estimate is kept per melody (nice-to-have; simple version: allow
  re-tap, firmware queues).

**Entry point:** new `soundsBtn` in `PadScreen.qml`'s side column (next to
`mouthsBtn`/`animsBtn`, ~line 459) emitting a new `melodyScreenRequested()`
signal; `main.qml` pushes `MelodyScreen.qml` on it (copy the
`mouthScreenRequested` pattern at `main.qml:30-33`). Icon: reuse an existing
music-ish asset or option A text button; asset can be added later.

## Screen 2: PianoScreen (note player)

**File:** `src/views/screens/PianoScreen.qml`

- `ScreenTemplate`, `screenName: "PianoScreen"`, title `tr("piano_title")`.
- **Keyboard UI**: one-octave piano drawn with QML `Rectangle`s — 7 white
  keys (C D E F G A B) + 5 black keys (C# D# F# G# A#) overlaid, classic
  layout. Pressed state = darker fill + small scale animation.
- **Octave selector**: row of buttons for octaves 3–6 (default 4, so
  A4 = 440 Hz). Frequency JS map mirroring `Zowi_sounds.h` note table
  (`note_C4: 261.63, note_Db4: 277.18, ...`); only the selected octave's 12
  semitone frequencies are needed (values rounded to int Hz for the `T`
  command, firmware `atoi()`s them).
- **Note duration**: selector with 3 presets — short (150 ms), medium
  (300 ms), long (600 ms). Short by default for playability given the
  blocking firmware behavior; long notes visibly delay subsequent taps.
- Key press (mouse/touch) → `Robot.sendData(Commands.tone(freq, duration))`.
- **Desktop keyboard support** (nice for a piano): `Keys`/`Shortcut` mapping
  computer keys to semitones (A→C, W→C#, S→D, E→D#, D→E, F→F, T→F#, G→G,
  Y→G#, H→A, U→A#, J→B), like web piano demos. `ScreenTemplate` focus needs
  checking (`forceActiveFocus()` on completion, as HomeScreen does).
- Optional cosmetic: show a mouth on Zowi's matrix while playing
  (e.g. `Commands.mouthById(Commands.MouthHappyOpen)` on key down) —
  decision: skip in v1, keep it sound-only.

**Entry point:** Home tile, like the Mouth Editor:
- `HomeScreen.qml`: add `{ name: tr("piano"), icon: ..., enabled: true }` to
  the apps list (~line 441) and a `pianoClicked()` signal + branch in the
  `onClicked` switch (~line 234). Icon: needs one new PNG
  (`piano_button.png`) or reuse an existing spare; text-tile fallback if no
  asset.
- `main.qml`: `home.pianoClicked.connect(...)` pushing `PianoScreen.qml`
  (copy the `mouthEditorClicked` pattern at `main.qml:39-42`).

## Shared implementation tasks (ordered)

1. **Expose tones to QML** — `CommandsController`:
   add `Q_INVOKABLE QString tone(int frequencyHz, int durationMs) const`
   wrapping `zowi::commandTone()` (declare in `.h` near `sing()`, implement in
   `.cpp`). No core change needed.
2. **Create `MelodyScreen.qml`** + wire `PadScreen` button/signal +
   `main.qml` navigation.
3. **Create `PianoScreen.qml`** + wire Home tile/signal + `main.qml`
   navigation.
4. **`views.qrc`**: add both new QML files (required for release builds;
   debug builds load from disk and hot-reload).
5. **i18n**: add keys to all 5 locales (`i18n/zowi_*.json`):
   - HomeScreen context: `piano` (tile name).
   - `MelodyScreen.qml` context: `melodies_title` + 19 melody labels
     (`melody_connection`, `melody_surprise`, ... — translated names, e.g.
     es: "Conexión", "Sorpresa", "Pedo 1"...).
   - `PianoScreen.qml` context: `piano_title`, `octave`, note/duration labels
     (`duration_short`, `duration_medium`, `duration_long`).
   - Context names must match the QML file names (TranslationEngine rule).
6. **Docs**: `docs/project/screens/SCREEN_MELODY.md` and
   `docs/project/screens/SCREEN_PIANO.md` following the existing per-screen
   doc format; add rows to `docs/project/screens/INDEX.md`.
7. **Preview scripts**: `src/views/tests/preview-melody.sh` and
   `preview-piano.sh` (copy `preview-moutheditor.sh`, `--connected` mode);
   add rows to `src/views/tests/README.md` table.
8. **Verification**:
   - `ctest --test-dir build --output-on-failure` (core untouched, but cheap).
   - Manual: debug build (`./build.sh --gui`), hot-reload the screens, play
     every melody and a few notes over BT and USB; confirm the `zowi.home()`
     servo-recenter side effect is acceptable UX (it is part of the firmware
     behavior for both `K` and `T`).
   - `scripts/verify_arduino_mirrors.sh` still passes (melody order untouched).

## Out of scope / future ideas

- Recording and playback of piano sequences (a real "song editor").
- Streaming multi-note chords — the firmware plays tones strictly
  sequentially; no polyphony.
- New melody content — melodies live in the firmware (`Zowi_sounds.h`); the
  app can only trigger the 19 existing ones (arbitrary songs would need a
  client-side sequencer sending many short `T` commands, which the blocking
  behavior makes clumsy — revisit if ever needed).
- PNG button assets for melodies/piano (option A ships without them).
