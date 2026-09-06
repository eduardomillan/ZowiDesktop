# SCREEN_MOUTH_EDITOR — MouthEditorScreen.qml

> "Pintabocas" mouth editor: draw Zowi's mouth dot-by-dot on a 6×5 LED grid;
> every change is sent live to the robot as an `L` command.

- **File:** `src/views/screens/MouthEditorScreen.qml`
- **i18n context:** `"MouthEditorScreen.qml"`
- Pushed by `HomeScreen.mouthEditorClicked`. Not part of the PadScreen flow.

## Signals

- None declared; navigation uses `ScreenTemplate`'s `backClicked`.

## QML context used

- `Robot`: `connected`, `sendData(cmd)`, `setDataPollingEnabled(false/true)`.
- `Commands`: `mouth(matrix)` — builds `L <32 bits>\r`.
- `Config.get(...)`: theme colors.

## Commands sent

| Action | Builder | Wire command |
|--------|---------|--------------|
| Draw / erase a cell | `Commands.mouth(matrix)` | `L 00<30 bits>\r` (grid → 32-bit pattern, sent on every change) |

## Implementation notes

- Grid cell `i` maps to bit `(29 - i)` of the 32-bit pattern; the two top bits
  stay zero (the matrix only has 30 LEDs).
- One `MouseArea` over the whole grid with `preventStealing: true`: press
  toggles the cell, drag paints/erases according to the last press state
  (`handleCellPress`/`handleCellDrag`, mirroring the Android
  `MouthGridLayout.handleTouch`).
- `footerHeight: 72` with Clear All / Select All buttons.
- `Component.onCompleted` disables the identity poll
  (`setDataPollingEnabled(false)`) so the `E/I/B` burst never interrupts the
  live mouth stream; re-enabled on destruction.