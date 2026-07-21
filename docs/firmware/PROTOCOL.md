# Zowi Firmware — Serial Command Protocol

This document is the authoritative reference for the commands the `zowi_cli`
sends to the robot over Bluetooth (SPP) / serial. It was derived from the
original BQ/bitbloq firmware in `zowiLibs/code .ino/ZOWI_BASE_v2/ZOWI_BASE_v2.ino`
and its `ZowiSerialCommand` parser.

## Framing

- Commands are **ASCII text**, tokens separated by a single **space** (`' '`).
- Each command is terminated by a **carriage return** (`\r`). A trailing `\n`
  is tolerated by the firmware (it is ignored as non-printable), so `\r\n`
  also works. The command builder in `src/core` emits `\r` only.
- Maximum command length is 35 bytes (`SERIALCOMMANDBUFFER`).
- Only printable characters are buffered; never send raw control bytes other
  than the `\r` terminator.

General form:

```text
<LETTER> [arg1] [arg2] ...\r
```

## Responses (robot → host)

Every response is framed with `&&` ... `%%` and followed by `\r\n`:

```text
&&A%%        # software ack (command received)
&&F%%        # final ack (action completed, e.g. EEPROM write done)
&&E <name>%% # robot name
&&I <appId>%%# program/app id
&&B <bat>%%  # battery level (percent)
&&D <cm>%%   # ultrasonic distance
&&N <noise>%%# noise level
```

The CLI's `onDataReceived` parser already understands this framing.

## Movement command `M`

```text
M <MoveID> <T> [<MoveSize>]\r
```

| MoveID | Meaning        | Firmware call            |
|--------|----------------|--------------------------|
| 0      | Stop / home    | `zowi.home()`            |
| 1      | Walk forward   | `zowi.walk(1, T, 1)`     |
| 2      | Walk backward  | `zowi.walk(1, T, -1)`    |
| 3      | Turn left      | `zowi.turn(1, T, 1)`     |
| 4      | Turn right     | `zowi.turn(1, T, -1)`    |
| 5      | Up/down        | `zowi.updown(1, T, size)`|
| 6      | Moonwalker L   | `zowi.moonwalker(1,T,size,1)` |
| 7      | Moonwalker R   | `zowi.moonwalker(1,T,size,-1)`|
| 8      | Swing          | `zowi.swing(1,T,size)`   |
| 9      | Crusaito F     | `zowi.crusaito(1,T,size,1)` |
| 10     | Crusaito B     | `zowi.crusaito(1,T,size,-1)`|
| 11     | Jump           | `zowi.jump(1,T)`         |
| 12     | Flapping L     | `zowi.flapping(1,T,size,1)` |
| 13     | Flapping R     | `zowi.flapping(1,T,size,-1)`|
| 14     | Tiptoe swing   | `zowi.tiptoeSwing(1,T,size)` |
| 15     | Bend F         | `zowi.bend(1,T,1)`       |
| 16     | Bend B         | `zowi.bend(1,T,-1)`      |
| 17     | Shake leg L    | `zowi.shakeLeg(1,T,1)`   |
| 18     | Shake leg R    | `zowi.shakeLeg(1,T,-1)`  |
| 19     | Jitter         | `zowi.jitter(1,T,size)`  |
| 20     | Ascending turn | `zowi.ascendingTurn(1,T,size)` |

- `T` — period in milliseconds. Larger = slower. Default `1000`.
- `MoveSize` — amplitude/height in degrees (e.g. `SMALL 5`, `MEDIUM 15`,
  `BIG 30`). Only used by the gesture/height movements; ignored for the four
  directional moves (1–4).

The firmware runs **one gait cycle per command**. To keep moving, re-send the
command. This is exactly what the interactive `control` minigame does: each
arrow-key press sends one `M ...` command.

## Stop command `S`

```text
S\r
```

Equivalent to `M 0\r`: moves all servos to 90°, detaches them, and puts the
robot at rest. Any unrecognized command is also treated as a stop by the
firmware's default handler.

## Example session

```text
M 1 1000\r       # walk forward
M 2 1000\r       # walk backward
M 6 1000 30\r    # moonwalker left (default left)
M 7 1000 30\r    # moonwalker right (default right)
M 3 1000\r       # turn left (tighter arc)
M 4 1000\r       # turn right (tighter arc)
S\r              # stop / home
```

## Other commands (reference)

| Command            | Description                          |
|--------------------|--------------------------------------|
| `R <name>\r`       | Set robot name (saved to EEPROM)     |
| `E\r`              | Request name → `&&E ...%%`           |
| `I\r`              | Request program id → `&&I ...%%`     |
| `B\r`              | Request battery → `&&B ...%%`        |
| `D\r`              | Request distance → `&&D ...%%`       |
| `N\r`              | Request noise → `&&N ...%%`          |
| `T <freq> <ms>\r`  | Buzzer tone                          |
| `H <GestureID>\r`  | Play a gesture                       |
| `K <SingID>\r`     | Play a song                          |
| `G <YL> <YR> <RL> <RR>\r` | Set raw servo angles (degrees) |
| `C <YL> <YR> <RL> <RR>\r` | Set servo trim offsets         |
