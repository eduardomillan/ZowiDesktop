# Relationship with zowiLibs

[zowiLibs](https://github.com/eduardomillan/zowiLibs) is ZowiDesktop's **bro
project**: it contains the firmware that runs on the Zowi robot (the original BQ
version, adapted by the same author) and the Arduino libraries that support it.
Both projects live in separate, independent GitHub repositories.

## What zowiLibs provides to ZowiDesktop

| Component | In zowiLibs | In ZowiDesktop | Relationship |
|---|---|---|---|
| Firmware `ZOWI_BASE_v2.hex` | `code .hex/ZOWI_BASE_v2.hex` | `src/firmware/ZOWI_BASE_v2.hex` | Byte-identical (differ only in CRLF→LF). Copied and normalised by `scripts/sync_firmware_from_zowiLibs.sh`. |
| Firmware `ZOWI_Alarm_v2.hex` | compiled from `code .ino/games/ZOWI_Alarm_v2/` | `src/firmware/ZOWI_Alarm_v2.hex` | Derived from the zowiLibs source. |
| Communication protocol | `ZowiSerialCommand` (`&&`/`%%`) with commands `S/L/T/M/H/K/C/G/R/E/D/N/B/I/A/F` | `src/core/include/zowi/protocol.h` | The `protocol.h` header defines the constants (**single source of truth**) and `makeCommand()` for building host-side commands. |
| Arduino libraries (Zowi, Oscillator, LedMatrix, US, BatReader, EnableInterrupt) | `arduino libraries/` | — | **Not reused.** These are AVR/Arduino-specific and run on the robot, not in the desktop application. |

## Arduino robot libraries (not reusable)

The libraries in `arduino libraries/` implement the robot's physical behaviour:
servo control (Oscillator, Zowi), LED matrix (LedMatrix), ultrasound sensor (US),
battery reading (BatReader) and external interrupts (EnableInterrupt). They are
written for the Arduino/AVR ecosystem and depend on hardware primitives
(`<Servo.h>`, `EEPROM`, `analogRead`, `digitalWrite`, etc.) that do not exist in
a desktop application.

ZowiDesktop models the robot through its own abstraction layer: `DeviceInfo`,
`BluetoothApi` and `robot_commands.h`. It does not attempt to compile native AVR
code.

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
mechanism for staying in sync with the canonical source.

## Privileges

| Area | Backend | How it connects | Needs `CAP_NET_ADMIN`? |
|---|---|---|---|
| Robot control (scan, connect, status, rename, movement, etc.) | `QtBluetoothBackend` (`src/backends/bt_qt/`) | Qt Bluetooth → [BlueZ](https://www.bluez.org/) D-Bus API → `bluetoothd` (system daemon) | **No.** BlueZ mediates all RFCOMM SPP connections via D-Bus and is already running as root, so a normal user can scan and connect without extra privileges. The same backend serves both the CLI and the GUI, which is why `connect`/`rename`/`status` work without `sudo`. |
| Firmware flashing (default, `--backend bluetooth`) | `QtBluetoothBackend` (`src/backends/bt_qt/`) | Qt Bluetooth → BlueZ D-Bus → RFCOMM SPP (same as control) | **No.** The same BlueZ SPP connection triggers the HC-05 STATE-pin reset, so flashing works root-free. This is the recommended and default path. |
| Firmware flashing (fallback, `--backend serial` / `--tty`) | `SerialBluetoothBackend` (`src/backends/bt_serial/`) | Opens `/dev/rfcomm*` directly + `ioctl` DTR pulse | **Yes.** Creating the RFCOMM TTY device with `rfcomm bind` requires `CAP_NET_ADMIN` (or root). Used when `--tty` is given or `--backend serial` is explicitly requested. |

### Design rule

**The default BlueZ SPP flashing path runs without extra privileges,** so the CLI
and (in the future) the GUI can flash firmware using the same backend that
handles robot control. The serial (RFCOMM TTY) backend is kept as a Linux-only
fallback for environments where BlueZ SPP is unavailable or the user prefers the
classic Arduino‑style DTR‑pulse approach.

