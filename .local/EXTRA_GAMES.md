# Extra Game Ideas for ZowiDesktop

## Table of contents

1. [Blow the Candles! (microphone + buzzer)](#1-blow-the-candles-microphone--buzzer)
2. [Zowi Radar / Get Closer (ultrasonic + LEDs + buzzer)](#2-zowi-radar--get-closer-ultrasonic--leds--buzzer)
3. [Musical Dance / Statues (melodies + movement + sensors)](#3-musical-dance--statues-melodies--movement--sensors)
4. [Zowi Maze (gamepad steering + mirrored movement)](#4-zowi-maze-gamepad-steering--mirrored-movement)
5. [Quick Draw! (reaction-time duel)](#5-quick-draw-reaction-time-duel)
6. [Guess the Number (deduction with expressive feedback)](#6-guess-the-number-deduction-with-expressive-feedback)
7. [Zowi Pet (virtual-pet care)](#7-zowi-pet-virtual-pet-care)
8. [Zowi Quiz (trivia with physical answers)](#8-zowi-quiz-trivia-with-physical-answers)
9. [LED Snake (arcade on the robot's face)](#9-led-snake-arcade-on-the-robots-face)
10. [Rock, Paper, Scissors (versus Zowi)](#10-rock-paper-scissors-versus-zowi)
- [Implementation notes](#implementation-notes)

---

Ten new game proposals that don't overlap with the existing (designed) games:
"1, 2, 3 ¡Acción!" (timeline/sequences), "Memoria" / Zowi Dice (Simon), and
"Pintabocas" (draw the mouth). They span distinct genres and, where possible,
leverage hardware capabilities that no current or planned game uses: the
noise/microphone sensor (`N`), the ultrasonic distance sensor (`D`), the
free-tone buzzer (`T`), and the melodies (`K`).

## 1. "Blow the Candles!" (microphone + buzzer)

Zowi shows a "cake" on the LED matrix with N lit candles and plays a birthday
melody (`K`). The player must **blow or shout** near Zowi; the app polls the
noise sensor (`N`) and every time the level exceeds a threshold, one candle
goes out (with a descending buzzer tone `T` as feedback). Blow out all candles
before the melody ends to win (VICTORY gesture); otherwise Zowi gets angry
(ANGRY). Progressive difficulty: more candles, higher noise threshold, shorter
melody.

- Unique angle: the only game that would use the microphone.
- Mechanics: timing + physical interaction, unlike sequences/memory/drawing.

## 2. "Zowi Radar" / "Get Closer" (ultrasonic + LEDs + buzzer)

A **theremin/proximity** game: Zowi measures hand distance with the ultrasonic
sensor (`D`) in a loop. The LED matrix shows a target that "fills up" as the
hand gets closer, and the buzzer emits increasingly higher-pitched tones (`T`)
— like a radar or a theremin.

Game mechanic: the app picks a random target distance ("hold your hand at 15
cm!") and the player must keep it within a ±2 cm band for 2 seconds to score.
Levels get harder with trickier targets and narrower bands.

- Unique angle: uses the distance sensor, which no screen currently consumes.

## 3. "Musical Dance" / "Statues" (melodies + movement + sensors)

Zowi **dances alone** through a random choreography while a melody plays
(`K` + movements `M`), and the player plays physically with it:

- **Statues variant**: when the music stops abruptly, the player must freeze —
  detected via the ultrasonic sensor (if `D` reports movement/distance change,
  the player loses a life).
- **Imitation variant**: replicate the dance Zowi just performed using the
  existing Gamepad as a controller, within 10 s. The app compares the sent
  commands against the danced sequence.

- Unique angle: combines audio + movement + sensors, distinct from "Memoria"
  (which is button-only Simon without music or sensors).

## 4. "Zowi Maze" (gamepad steering + mirrored movement)

A **labyrinth/steering** game: a randomly generated maze is drawn on screen
with Zowi as the avatar. The player drives it with the existing Gamepad
controls, and the real robot mirrors every turn/step physically on the table.
Walls cost a life; reaching the exit before the timer runs out scores points.
Levels grow bigger and add moving obstacles.

- Unique angle: spatial navigation with the Gamepad as a real-time controller,
  not sequence building or button-repeating.

## 5. "Quick Draw!" (reaction-time duel)

A **reflex** game: Zowi stands idle for a random delay (2–8 s) showing a
"waiting" face on the LED matrix, then suddenly jumps (`M 14`) and buzzes.
The player must tap the screen as fast as possible. Tapping before the signal
is a false start (Zowi shows ANGRY). Score = reaction time in ms, with a local
best-time leaderboard. Rounds are best-of-5 against Zowi's "own" random time.

- Unique angle: pure reaction speed — no memory, no sequences, no sensors.

## 6. "Guess the Number" (deduction with expressive feedback)

A **higher/lower deduction** game: Zowi thinks of a secret number (1–100).
The player guesses on screen; Zowi answers *expressively* instead of with
text — tilts up (bendBackward) + rising tone for "higher", tilts down +
falling tone for "lower", VICTORY + melody on a hit. Fewer guesses = more
points (par is 7, the binary-search optimum). LED matrix shows a hot/cold
meter.

- Unique angle: logic/deduction genre; uses gestures and tones as the game
  language rather than as decoration.

## 7. "Zowi Pet" (virtual-pet care)

A **tamagotchi-style** game that persists across sessions: Zowi has mood,
hunger and energy stats stored in the session/config. The player "feeds" it
(melodies `K`), "plays" with it (movements via quick commands) and puts it to
"sleep" (lights off, no polling). Neglect drops the mood and the LED mouth
changes (happy → sad → angry). Daily check-in streaks unlock cosmetics
(new default mouths).

- Unique angle: nurturing/persistence genre — a long-term companion loop, not
  a score-chasing minigame.

## 8. "Zowi Quiz" (trivia with physical answers)

A **trivia** game: questions (kids-friendly, localizable, bundled JSON
question packs) appear on screen with 4 options mapped to the 4 Gamepad
directions. The player answers by pressing the direction or moving the real
robot that way (walkLeft = A, walkForward = B...). Zowi nods/shakes its head
(gestures) for right/wrong answers, with buzzer tones. Timed rounds, streak
multipliers, VICTORY dance on a perfect run.

- Unique angle: knowledge genre; reuses the `QuizComponent` pattern from
  ProjectMoveScreen but as a standalone arcade game with physical answering.

## 9. "LED Snake" (arcade on the robot's face)

Classic **Snake rendered on Zowi's 6×5 LED matrix**: the snake (raw 30-bit
patterns via `L`) crawls on Zowi's actual face while a larger mirrored board
is shown on screen. Steering with Gamepad arrows; eating dots scores and
lengthens the snake; hitting walls/yourself ends the run with a FAIL gesture.
Speed increases with score.

- Unique angle: turns the LED matrix into a live game display (vs. Pintabocas,
  where the matrix is a static pattern to copy).

## 10. "Rock, Paper, Scissors" (versus Zowi)

A **head-to-head chance/mind-game**: the player picks rock, paper or scissors
on screen; Zowi simultaneously "throws" its choice using a gesture (e.g.
Happy = paper/open hand, Angry = rock/fist, a mouth pattern = scissors) plus a
buzzer tone. Score, best-of-N rounds and a running tally. Optional twist:
after several rounds Zowi shows a "tell" (a brief LED flicker hinting at its
throw) for observant players.

- Unique angle: versus-the-robot genre with instant rounds; purely
  presentational use of gestures/mouths, no copying or memorizing.

## Implementation notes

- Expose in `CommandsController` what already exists in core but doesn't reach
  QML: `commandTone()` (`src/core/include/zowi/protocol.h`) and the `D`/`N`
  parsing in `MessageParser` (`src/core/src/message_parser.cpp`, already
  covered by tests).
- Register the tiles in `HomeScreen.qml` (games `ListModel`, ~lines 440-448)
  with translated names in `i18n/zowi_es_ES.json` (and the other locales).
- Follow the design-doc convention: `docs/project/screens/SCREEN_GAME_*.md`
  (the three existing games are also design-only there, not yet implemented).
- Each game screen is a `ScreenTemplate`; i18n context = QML file name;
  navigation wired via signals in `src/views/main.qml` (StackView).
