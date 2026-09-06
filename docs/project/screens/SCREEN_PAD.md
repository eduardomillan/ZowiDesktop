# SCREEN_PAD — PadScreen.qml

> Interactive gamepad: movement pad (left), action pad (right) and the central
> column **mouths → animations → speed**. It is the main live-control screen,
> equivalent to the Android app's gamepad.

- **File:** `src/views/screens/PadScreen.qml`
- **i18n context:** `"PadScreen.qml"`
- Pushed by `HomeScreen.gamepadClicked`.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `mouthScreenRequested()` | mouths button | `main.qml`: push→ Mouth (MouthScreen.qml) |
| `gestureScreenRequested()` | animations button | `main.qml`: push→ Gesture (GestureScreen.qml) |
| `backClicked()` | inherited from `ScreenTemplate` | `main.qml`: `stack.pop()` |

## QML context used

- `Robot`: `connected`, `sendData(cmd)`, `setDataPollingEnabled(false/true)`.
- `Commands`: the builder object — every movement/stop command goes through it.
- `Config.get(...)`: theme colors (`color_bg_connected`, `color_primary`).
- `Translator` (via `tr()`); speed names come from `PadScreen.qml` keys.

## Commands sent

| Control | Builder | Wire command (medium speed) |
|---------|---------|------------------------------|
| Up | `Commands.walkForward(speed)` | `M 1 <T>\r` |
| Down | `Commands.walkBackward(speed)` | `M 2 <T>\r` |
| Turn left / right | `Commands.turnLeft/Right(speed)` | `M 3/4 <T>\r` |
| Moonwalker left / right | `Commands.moonwalkerLeft/Right(speed, 30)` | `M 6/7 <T> 30\r` |
| Bend | `Commands.bendForward(speed)` | `M 15 <T>\r` |
| Shake leg | `Commands.shakeLegLeft(speed)` | `M 17 <T>\r` |
| Up/down | `Commands.updown(speed, 15)` | `M 5 <T> 15\r` |
| Jitter | `Commands.jitter(speed, 15)` | `M 19 <T> 15\r` |
| Swing | `Commands.swing(speed, 15)` | `M 8 <T> 15\r` |
| Flapping | `Commands.flappingLeft(speed, 30)` | `M 12 <T> 30\r` |
| Crusaito | `Commands.crusaitoForward(speed, 30)` | `M 9 <T> 30\r` |
| Stop (on release) | `Commands.stop()` | `S\r` |

All **hold-repeating**: `startHold()` sends immediately and restarts
`repeatTimer` (200 ms) while held; `stopHold()` stops the timer and sends `S`.

## Implementation notes

- **Speed modes** (`MovementSpeed`): slow = 2000 ms, medium = 1000 ms, fast =
  700 ms (larger period = slower gait). The speed button is an overlay of three
  images toggled by `visible`.
- Central column order (top → bottom): **mouthsBtn → animsBtn → speedControl**.
- `Component.onCompleted`: `Robot.setDataPollingEnabled(true)` (PadScreen
  pauses the poll while a button is held so the `E/I/B` burst never interrupts
  a movement — same pattern as CalibrationScreen).

## Related pickers

- Mouths button → [SCREEN_MOUTH.md](SCREEN_MOUTH.md)
- Animations button → [SCREEN_GESTURE.md](SCREEN_GESTURE.md)