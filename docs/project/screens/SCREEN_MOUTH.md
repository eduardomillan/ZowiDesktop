# SCREEN_MOUTH — MouthScreen.qml

> Mouth picker: a grid of 20 LED-matrix expressions. Tapping one sends the
> corresponding `L` command; the expression stays until changed.

- **File:** `src/views/screens/MouthScreen.qml`
- **i18n context:** `"MouthScreen.qml"`
- Pushed by `PadScreen.mouthScreenRequested` (mouths button of the gamepad).

## Signals

- None declared; navigation uses `ScreenTemplate`'s `backClicked`.

## QML context used

- `Robot`: `connected`, `sendData(cmd)`, `setDataPollingEnabled(false/true)`.
- `Commands`: the `Mouth*` ID constants and `mouthById(id)`.
- `Config.get(...)`: theme colors.

## Commands sent

| Action | Builder | Wire command |
|--------|---------|--------------|
| Tap a mouth | `Commands.mouthById(id)` | `L <32 bits>\r` (pattern looked up from the core `kMouthPatterns`) |

## Mouth grid

20 entries (`MouthId` enum, protocol 0-based): Smile, HappyOpen, Heart,
BigSurprise, SmallSurprise, TongueOut, Vamp1, Vamp2, LineMouth, Confused,
Diagonal, Sad, SadOpen, SadClosed, Ok, X, Interrogation, Thunder, Culito,
Angry. (Indices 0-9 — the "zero".."nine" digits — have no icon asset and are
not shown.)

## Implementation notes

- `mouthIdByName` maps the QML grid names to `Commands.Mouth*` constants;
  `mouthOptions` holds the normal/pressed image pairs.
- Tapping a cell swaps to the pressed image on press and sends on release.
- Pauses the identity poll while open (`setDataPollingEnabled(false)`),
  restoring it on destruction — the same live-stream convention as
  Calibration and MouthEditor.