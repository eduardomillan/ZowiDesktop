# Relationship with zowiLibs

[zowiLibs](https://github.com/eduardomillan/zowiLibs) is ZowiDesktop's **bro
project**: it contains the firmware that runs on the Zowi robot (the original BQ
version, adapted by the same author) and the Arduino libraries that support it.
Both projects live in separate, independent GitHub repositories.

## Contents

- [What zowiLibs provides to ZowiDesktop](#what-zowilibs-provides-to-zowidesktop)
- [Arduino robot libraries (not reusable)](#arduino-robot-libraries-not-reusable)
- [Mirroring the Arduino libraries in ZowiDesktop](#mirroring-the-arduino-libraries-in-zowidesktop)
- [Virtual Zowi (simulator)](#virtual-zowi-simulator)
- [Protocol table](#protocol-table)
- [Keeping the HEX files in sync](#keeping-the-hex-files-in-sync)
- [Privileges](#privileges)

## What zowiLibs provides to ZowiDesktop

| Component | In zowiLibs | In ZowiDesktop | Relationship |
|---|---|---|---|
| Firmware `ZOWI_BASE_v2.hex` | `code/base/ZOWI_BASE_v2.hex` | `src/firmware/ZOWI_BASE_v2.hex` | Byte-identical (differ only in CRLF→LF). Copied and normalised by `scripts/sync_firmware_from_zowiLibs.sh`. |
| Firmware `ZOWI_Alarm_v2.hex` | `code/games/ZOWI_Alarm_v2/ZOWI_Alarm_v2.hex` | `src/firmware/ZOWI_Alarm_v2.hex` | Byte-identical (differ only in CRLF→LF). Copied and normalised by the sync script. |
| Firmware `ZOWI_Adivinawi_v2.hex` | `code/games/ZOWI_Adivinawi_v2/ZOWI_Adivinawi_v2.hex` | `src/firmware/ZOWI_Adivinawi_v2.hex` | Byte-identical (differ only in CRLF→LF). Copied and normalised by the sync script. |
| Communication protocol | `ZowiSerialCommand` (`&&`/`%%`) with commands `S/L/T/M/H/K/C/G/R/E/D/N/B/I/A/F` | `src/core/include/zowi/protocol.h` | The `protocol.h` header defines the constants (**single source of truth**) and `makeCommand()` for building host-side commands. |
| Arduino libraries (Zowi, Oscillator, LedMatrix, US, BatReader, EnableInterrupt) | `arduinolibs/` | — | **Not reused as code.** These are AVR/Arduino-specific and run on the robot, not in the desktop application. What *is* mirrored is their protocol and data catalogs — see [Mirroring the Arduino libraries](#mirroring-the-arduino-libraries-in-zowidesktop) and [Virtual Zowi](#virtual-zowi-simulator). |

## Arduino robot libraries (not reusable)

The libraries in `arduinolibs/` implement the robot's physical behaviour:
servo control (Oscillator, Zowi), LED matrix (LedMatrix), ultrasound sensor (US),
battery reading (BatReader) and external interrupts (EnableInterrupt). They are
written for the Arduino/AVR ecosystem and depend on hardware primitives
(`<Servo.h>`, `EEPROM`, `analogRead`, `digitalWrite`, etc.) that do not exist in
a desktop application.

ZowiDesktop models the robot through its own abstraction layer: `DeviceInfo`,
`BluetoothApi` and `robot_commands.h`. It does not attempt to compile native AVR
code.

## Mirroring the Arduino libraries in ZowiDesktop

"Mirror" here means: the desktop side implements the **host** half of the
contract that `arduinolibs` + the firmware sketches implement on the **robot**
side. The libraries themselves are never linked or ported (see previous
section); what gets mirrored is the observable protocol and its data catalogs.

### Current state of the mirror

| Arduino side (`zowiLibs/arduinolibs`) | ZowiDesktop core | Status |
|---|---|---|
| `ZowiSerialCommand` framing (35-byte buffer, `' '` delimiter, `'\r'` terminator, printable-only) | `protocol.h`: `makeCommand()`, `kMessagePrefix`/`kMessageTerminator` | ✅ High fidelity |
| Command table `S L T M H K C G R E D N B I` (14 commands) | `Command` enum in `protocol.h` | ✅ Complete |
| `M <id> <T> [size]` / firmware `move()` switch (0–20) | `robot_commands.cpp` builders (walk, turn, moonwalker, ...) | ✅ Matches the firmware switch exactly |
| `H <id>` gestures 1–13 (`Zowi_gestures.h`) | `GestureId` (0-based enum; command = id+1) | ✅ |
| `K <id>` melodies 1–19 (`Zowi_sounds.h`) | `MelodyId` | ✅ On the wire — see note on ordering below |
| `L <binary>` / `getMouthShape()`: 31 mouths (`Zowi_mouths.h`) | `kMouthPatterns` table + `MouthId` | ✅ (okMouth reverted to the canonical pattern, see below) |
| `T <freq> <ms>` buzzer | `commandTone(freq, ms)` | ✅ |
| Host-side response parser | `zowi::MessageParser` (`message_parser.h`), shared by CLI and GUI | ✅ Single parser, unit-tested |
| `putAnimationMouth()` (littleUuh, dreamMouth, adivinawi, wave) | — | ✅ Correctly omitted: not reachable over serial in `ZOWI_BASE_v2` (internal to games) |

### Known divergences and notes

**okMouth (index 25).** `robot_commands.cpp` carries a "corrected" pattern
(`0b00000000010000100101000010000000`) that deviates from the Arduino source.
Verification (2026-09): the original `0b00000001000010010100001000000000` is
identical in the local library, the canonical `bq/zowiLibs` upstream and the
Bobwi fork; the author's own design notes (`.local/PADSCREEN_IMPLEMENTING.md`)
also listed the original; and rendering through the calibrated `LedMatrix` bit
layout (see below) shows the original draws a coherent check-mark shape while
the "corrected" one produces an asymmetric blob. **Resolved and
hardware-confirmed (2026-09)** on the real robot: the canonical pattern was
restored (locked by `test_robot_commands`). The verification used two
single-dot probes (`mouth 0010...0` lit the top-left corner, `mouth ...0001`
the bottom-right), which validated the `LedMatrix` bit layout end-to-end and
confirmed the canonical pattern renders as a legible ✓ check mark (long arm
ascending to the right). The initial impression of the canonical pattern was
"incoherent blob" — 5 sparse dots are hard to read without context; the
orientation probes settled it. The CLI's `mouth` command (one-shot and shell)
accepts a raw 32-bit `0/1` pattern since this verification, mirroring the
firmware's `L` capability; it is the tool for any future on-hardware mouth
debugging.

**MelodyId ordering.** The desktop enum follows the order of the firmware's
`receiveSing()` switch (`K 1`→`S_connection` ... `K 19`→`S_buttonPushed`), **not**
the raw order of the `S_*` defines in `Zowi_sounds.h` (connection,
disconnection, buttonPushed, mode1-3, surprise, ...). Correct on the wire;
documented here to avoid confusion when mirroring the catalogs.

**LedMatrix bit layout** (reference for rendering mouths, e.g. the virtual
Zowi): 5 rows × 6 columns; bit 29 = row 1 / column 1, bit 0 = row 5 / column 6
(bits 30–31 unused). Validated by rendering `smile`, `sad`, `xMouth`, `heart`
and `diagonal`, which produce their expected shapes under this mapping.

### Unification roadmap (completed 2026-09)

1. **`MessageParser` in `src/core`** — ✅ Done. Host-side inverse of
   `ZowiSerialCommand` (`message_parser.h`): incremental `feed()` + typed
   `RobotMessage` list, covering `&&<cmd>[ <value>]%%` (including the bare
   `&&A%%`/`&&F%%` forms the firmware emits), the legacy `N`/`U`/`B` line
   forms, partial frames and noise, with a 4 KB resync valve against broken
   streams. Unit-tested in `src/core/tests/test_message_parser.cpp`.
2. **Unify consumers** — ✅ Done. `cli_state.cpp` and
   `RobotController::parseIncoming` feed the shared parser instead of
   hand-rolling two; the GUI now receives the same coverage (acks, distance,
   noise) and the parser resets when the backend is rebuilt. STK500 upload
   buffering stays outside the parser.
3. **Complete the builders** — ✅ Done. `commandTone(freq, ms)` →
   `T <freq> <ms>\r`; `okMouth` reverted to the canonical pattern (locked by
   test); `MelodyId` ordering rationale documented in the header.
4. **Catalog verification script** — ✅ Done. `scripts/verify_arduino_mirrors.sh`
   extracts mouths from `Zowi_mouths.h`/`Zowi.cpp`, gestures from
   `Zowi_gestures.h`, the `K`-case order from `ZOWI_BASE_v2.ino` and the
   `addCommand` letters, and diffs them against the core constants; non-zero
   exit on drift. Run it after any firmware change (complement of
   `sync_firmware_from_zowiLibs.sh`).

## Virtual Zowi (simulator)

A **virtual Zowi** is a device that implements the *other end* of the serial
protocol without hardware: it consumes the same commands
(`S/L/T/M/H/K/C/G/R/E/D/N/B/I`) and emits the same `&&...%%` responses as the
real firmware, so GUI and CLI can connect to it like to a real robot.

### Why it does not reuse the robot libraries

`Oscillator`, `LedMatrix`, `US` and `BatReader` are the *implementation* of the
behaviour behind the commands (servo oscillation, 74HC595 bit-banging,
ultrasound echo timing, ADC battery reading), tied to AVR primitives
(`<Servo.h>`, `digitalWrite`, `pulseIn`, `analogRead`, EEPROM) that do not
exist on a desktop. A simulator does not port that code — it imitates only its
**observable behaviour** through the protocol. The desktop never needs to
compute servo angles; it only speaks the contract.

### Layers

| Layer | What it mirrors | Effort | Value |
|---|---|---|---|
| **Minimal** — `VirtualZowiBackend : BluetoothApi` | In-memory state (name, appId `ZOWI_BASE_v2`, battery, trims) + a robot-side command parser (the symmetric counterpart of the `MessageParser` roadmap item): acks `&&A%%`/`&&F%%`, canned/parameterised `&&E`/`&&I`/`&&B`/`&&D`/`&&N`, clean rejection of firmware flashing | ~150–250 lines backend + ~80–100 lines parser + deterministic core tests; **≈1.5–2.5 days** incl. wiring | UI development and protocol tests without the robot. The GUI has no demo equivalent today (`--demo` is only a CLI script) |
| **Medium** — `LedMatrix` mirror as a QML widget | 5×6 grid rendering the 32-bit mouth pattern. Cheap trick: the firmware never pushes mouth state over serial, so the widget renders the last `L` command sent by `CommandsController` locally — no backend changes needed | ~0.5 day | Develop MouthsDialog / mouth editor without hardware |
| **Full** — `Oscillator` mirror | Gait math (sinusoidal servo interpolation + per-movement phase tables from `Zowi.cpp`) animating an on-screen figure | 2–4 days, high maintenance | Low today: the desktop executes movements on the real robot, it does not visualise them. **Avoid for now** |

### Integration notes

- **CLI**: add `"virtual"` to the `--backend` option of the subcommands and to
  the backend factory (`cli_util.cpp`) — mechanical (~10 subcommands).
- **GUI**: the transport model is deliberately not user-selectable (the
  `situation` state machine derives the transport from availability +
  registration). A demo transport must be enclaved as a developer feature
  (build flag or hidden SettingsScreen toggle) rather than offered as a third
  user-visible transport.
- **Flashing**: the virtual backend must reject STK500 upload cleanly via
  `onError`.
- **Synergy**: the roadmap's `MessageParser` and `commandTone` items are
  natural prerequisites — the virtual backend enables a closed round-trip
  integration test (`makeCommand()` → VirtualZowi → `MessageParser`) with no
  hardware.

## Protocol table

| Direction | Command | Meaning | Reply |
|---|---|---|---|
| → robot | `S` | Stop / home | `&&A%%` `&&F%%` |
| → robot | `L <leds>` | LED matrix control | `&&A%%` `&&F%%` |
| → robot | `T <note>` | Buzzer | `&&A%%` `&&F%%` |
| → robot | `M <id> <T>` | Movement (1 walk, 2 backward, 3 turnL, 4 turnR) | `&&A%%` `&&F%%` |
| → robot | `H <id>` | Gesture | `&&A%%` `&&F%%` |
| → robot | `K <melody>` | Sing / melody | `&&A%%` `&&F%%` |
| → robot | `C <trims>` | Servo trim adjustment | `&&A%%` `&&F%%` |
| → robot | `G <servo> <angle>` | Direct servo control | `&&A%%` `&&F%%` |
| → robot | `R <name>` | Rename (writes EEPROM) | `&&A%%` `&&F%%` |
| → robot | `E` | Request name | `&&E <name>%%` |
| → robot | `D` | Request distance | `&&D <cm>%%` |
| → robot | `N` | Request noise | `&&N <value>%%` |
| → robot | `B` | Request battery | `&&B <%>%%` |
| → robot | `I` | Request program ID | `&&I <id>%%` |
| ← desktop | `&&A%%` | Ack (command received) | — |
| ← desktop | `&&F%%` | Final ack (command fully processed) | — |

*Legacy* (line-based, old firmware): `N <name>`, `U <id>`, `B <batt>`.

## Keeping the HEX files in sync

```bash
# 1. Clone zowiLibs (if you don't have it yet):
#    git clone https://github.com/eduardomillan/zowiLibs.git
# 2. Run the synchronisation script:
scripts/sync_firmware_from_zowiLibs.sh [/path/to/zowiLibs]
# 3. Review the diff, rebuild and test.
```

The script copies the HEX files from zowiLibs into `src/firmware/` and normalises
line endings (CRLF→LF). The HEX files remain committed in ZowiDesktop so the
project stays self-contained at build time; the script is the reproducible
mechanism for staying in sync with the canonical source. The protocol/data
catalogs (mouths, gestures, melodies, command letters) are mirrors-by-hand
rather than copies, so they are covered by the verification script
`scripts/verify_arduino_mirrors.sh` instead — run it whenever the firmware
sources change.

## Privileges

| Area | Backend | How it connects | Needs `CAP_NET_ADMIN`? |
|---|---|---|---|
| Robot control (scan, connect, status, rename, movement, etc.) | `QtBluetoothBackend` (`src/backends/bt_qt/`) | Qt Bluetooth → [BlueZ](https://www.bluez.org/) D-Bus API → `bluetoothd` (system daemon) | **No.** BlueZ mediates all RFCOMM SPP connections via D-Bus and is already running as root, so a normal user can scan and connect without extra privileges. The same backend serves both the CLI and the GUI, which is why `connect`/`rename`/`status` work without `sudo`. |
| Firmware flashing (default, `--backend bluetooth`) | `QtBluetoothBackend` (`src/backends/bt_qt/`) | Qt Bluetooth → BlueZ D-Bus → RFCOMM SPP (same as control) | **No.** The same BlueZ SPP connection triggers the HC-05 STATE-pin reset, so flashing works root-free. This is the recommended and default path. |
| Firmware flashing (fallback, `--backend serial` / `--tty`) | `SerialBluetoothBackend` (`src/backends/bt_serial/`) | Opens `/dev/rfcomm*` directly + `ioctl` DTR pulse | **Yes.** Creating the RFCOMM TTY device with `rfcomm bind` requires `CAP_NET_ADMIN` (or root). Used when `--tty` is given or `--backend serial` is explicitly requested. |
| Firmware flashing over USB (`--backend usb`) | `SerialBluetoothBackend` (`src/backends/bt_serial/`) | Opens a USB serial TTY (`/dev/ttyUSB*`, `/dev/ttyACM*`) directly + `ioctl` DTR pulse | **No.** No `rfcomm bind` is involved; only serial-device access is needed (e.g. the `dialout` group). For machines without Bluetooth. |

### Design rule

**The default BlueZ SPP flashing path runs without extra privileges,** so the CLI
and (in the future) the GUI can flash firmware using the same backend that
handles robot control. The serial (RFCOMM TTY) backend is kept as a Linux-only
fallback for environments where BlueZ SPP is unavailable or the user prefers the
classic Arduino‑style DTR‑pulse approach.

