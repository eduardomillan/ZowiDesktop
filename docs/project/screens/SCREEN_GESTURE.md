# SCREEN_GESTURE — GestureScreen.qml

> Gesture picker: a grid of 11 body-language animations. Tapping one plays it
> via the `H` command.

- **File:** `src/views/screens/GestureScreen.qml`
- **i18n context:** `"GestureScreen.qml"`
- Pushed by `PadScreen.gestureScreenRequested` (animations button of the
  gamepad).

## Signals

- None declared; navigation uses `ScreenTemplate`'s `backClicked`.

## QML context used

- `Robot`: `connected`, `sendData(cmd)`, `setDataPollingEnabled(false/true)`.
- `Commands`: the `Gesture*` ID constants and `gestureById(id)`.

## Commands sent

| Action | Builder | Wire command |
|--------|---------|--------------|
| Tap a gesture | `Commands.gestureById(id)` | `H <id+1>\r` (protocol 1-based) |

## Gesture grid

11 entries (`GestureId` enum): Happy, SuperHappy, Sad, Sleeping, Fart,
Confused, Love, Angry, Fretful, Magic, Wave. (Wire IDs 12 Victory and 13 Fail
are implemented in the core/CLI but have no icon asset, so they are not in the
grid.)

## Implementation notes

- `gestureIdByName` maps grid names to the 0-based `Commands.Gesture*`
  constants; `gestureById` adds 1 for the 1-based protocol.
- Tapping a cell swaps to the pressed image on press and sends on release.
- Pauses the identity poll while open (`setDataPollingEnabled(false)`),
  restoring it on destruction.