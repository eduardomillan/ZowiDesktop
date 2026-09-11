# SCREEN_SOUNDS — SoundsScreen.qml

> Melody picker: a grid of the 19 firmware melodies. Tapping one plays it via
> the `K` command. Each cell shows a badge icon at rest and Zowi's face while
> that melody is playing, until the robot's final ack (`&&F%%`).

- **File:** `src/views/screens/SoundsScreen.qml`
- **i18n context:** `"SoundsScreen.qml"`
- Pushed by `PadScreen.soundScreenRequested` (sounds button of the
  gamepad, under Gestures; icon `images/sounds/sound_button.png`).

## Signals

- None declared; navigation uses `ScreenTemplate`'s `backClicked`.

## QML context used

- `Robot`: `connected`, `sendData(cmd)`, `setDataPollingEnabled(false/true)`,
  `finalAckReceived`, `connectionChanged`.
- `Commands`: the `Melody*` ID constants and `sing(id)`.

## Commands sent

| Action | Builder | Wire command |
|--------|---------|--------------|
| Tap a melody | `Commands.sing(id)` | `K <id+1>\r` (protocol 1-based) |

## Melody grid

19 entries (`MelodyId` enum order, wire IDs 1–19): Connection, Disconnection,
Surprise, OhOoh, OhOoh2, Cuddly, Sleeping, Happy, SuperHappy, HappyShort, Sad,
Confused, Fart1, Fart2, Fart3, Mode1, Mode2, Mode3, ButtonPushed.

Assets live under `images/sounds/`:

- Rest: `melody_<slug>_badge.png`
- Playing: `melody_<slug>.png` (Zowi face)

## Implementation notes

- `melodyIdByName` maps grid names to the 0-based `Commands.Melody*`
  constants; `sing` adds 1 for the 1-based protocol.
- While `playingMelody` is set, other cells are dimmed and ignore taps.
- `Robot.finalAckReceived` (from `&&F%%`) clears `playingMelody` and restores
  the badge. Disconnect also clears the playing state.
- Pauses the identity poll while open (`setDataPollingEnabled(false)`),
  restoring it on destruction.
- Firmware side effect: `receiveSing` calls `zowi.home()` before playback.
