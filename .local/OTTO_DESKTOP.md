# OttoDesktop — Feasibility & v1 Plan

Companion app to ZowiDesktop for **Otto DIY** robots (Arduino Nano based).
Decided: **new standalone repo `OttoDesktop`**, **SPP + BLE + USB transports
from v1**, and **dual firmware support with runtime detection** (official
`Otto_APP.ino` v13 and our own patched fork).

## Table of contents

- [Feasibility summary](#feasibility-summary)
- [Otto vs. Zowi protocol differences](#otto-vs-zowi-protocol-differences)
- [Firmware strategy & detection](#firmware-strategy--detection)
- [Core (Qt-free, `otto::` namespace)](#core-qt-free-otto-namespace)
- [Backends](#backends)
- [GUI v1 screens](#gui-v1-screens)
- [Configuration & session keys](#configuration--session-keys)
- [Risks & open items](#risks--open-items)
- [Effort estimate](#effort-estimate)
- [References](#references)

---

## Feasibility summary

Yes, feasible — because **Otto and Zowi share
a common ancestor and (almost) the same wire protocol**. The official app
firmware (`OttoDIYLib` examples/`Otto_APP.ino`, v13) speaks the same ASCII
command set as Zowi: `S L T M H K C G` with `&&A%%` / `&&F%%` ACKs, the same
MoveIDs 1–20, gesture IDs 1–13 and melody IDs 1–19. ZowiDesktop's Qt-free core
(command builders, message parser, `MovementSequencer`) ports over almost
as-is; the deltas are the 8x8 mouth payload, missing sensor/name commands,
arms, and a serial-stream quirk (below).

## Otto vs. Zowi protocol differences

| Aspect | Zowi | Otto (official app firmware v13) |
|---|---|---|
| Mouth command `L` | 32-bit pattern, 6x5 matrix | **64-bit binary pattern, 8x8 MAX7219** (`strtoul(arg, 2)`) |
| Predefined mouths | 31 (6x5) | ~30 (8x8) in `Otto_mouths.h`, same names (smile, happyOpen...) |
| Movements `M` | IDs 1–20 | IDs 1–20, identical mapping |
| Gestures `H` | 1–13 | 1–13, identical order |
| Melodies `K` | 1–19 | 1–19, identical order (`S_connection`...`S_buttonPushed`) |
| Buzzer `T` | freq + ms | identical |
| Trims `C` / raw servos `G` | 4 servos | 4 servos, identical |
| Sensors `D`/`N`, battery `B`, name `E`/`R`/`I` | present | **absent** in official firmware |
| Arms | n/a | **absent** (4-servo firmware); humanoid variant exists only as library API (`Otto9Humanoid`, 6 servos, arms on pins 6/7) |
| ACK streams | same stream as commands | **quirk**: commands read only from BT SoftwareSerial (9600); ACKs printed only to USB `Serial` |
| Unknown commands | ignored | **default handler = receiveStop** (sends `&&A%%&&F%%` and homes) |

## Firmware strategy & detection

Two supported firmwares, detected at connect time:

- **Official `Otto_APP.ino` v13** (no flashing needed): full mouth/gesture/
  melody/movement/calibration control **over BT only**; no arms; no ACKs on
  the command stream, so sequencing uses **estimated durations** (the `T`
  parameter of each move gives its length) instead of `A`/`F` handshakes.
  USB is ACK-only (cannot drive the robot).
- **OttoDesktop fork** (flashed once over USB, lives in `firmware/` or its own
  repo): reads commands from **both** streams, prints ACKs to **both**, adds
  arm servos (pins 6/7) with extended MoveIDs (21+), a capabilities command
  `V` answering `&&V <flags>%%` (bits: arms, mouth, ultrasonic...), and
  optionally Zowi-style `D`/`N`/`B` sensor/battery queries.

**Detection handshake**: send `V` right after connect and wait briefly:
- `&&V <flags>%%` → fork (full feature set, ACK-based sequencing).
- `&&A%%&&F%%` (the official default handler firing `receiveStop`) → official
  firmware (degraded mode; probe is idle-safe because it only homes/stops).
- Nothing (USB + official) → USB cannot control; guide the user to BT or to
  flash the fork.

Capability flags and the user's `hasMouth` / `hasArms` toggles gate UI:
mouth screens hidden when no matrix, arm pad/calibration columns hidden when
no arms or official firmware.

## Core (Qt-free, `otto::` namespace)

- Port of Zowi core: `robot_commands` (identical ASCII builders; `L` payload
  widened to `unsigned long long` 64-bit), `message_parser` (`&&...%%`
  framer, plus `V` capabilities frame), `MovementSequencer` with two modes:
  **ACK mode** (fork) and **estimated-duration mode** (official).
- Mouth pattern table: 8x8 presets ported from `Otto_mouths.h` (64-bit),
  kept in sync with a verify script like `scripts/verify_arduino_mirrors.sh`.
- Situation state machine (Connected/Disconnected/TransportLost + transport
  bt/usb/ble), session/config stores, `TranslationEngine` — copied & trimmed.
- `ctest` suites for the 64-bit `L` builder, parser and capabilities decode.

## Backends

- **SPP classic** (HC-05/HC-06): Zowi's `bt_qt` backend reused as-is; baud
  9600 default, configurable (some modules ship at 57600).
- **BLE** (HM-10-style serial modules): new GATT backend
  (`QLowEnergyController`, write + notify), same design as the mBot plan;
  UUIDs configurable until confirmed on hardware.
- **USB serial**: Zowi's `bt_serial` backend reused; with official firmware
  it is receive-only (ACKs), with the fork it is full-duplex.
- Shared `BluetoothApi`-style abstraction so the controller stays
  transport-agnostic.

## GUI v1 screens

- **Splash** → **Welcome/Scan**: SPP + BLE discovery and USB port probe; no
  rename wizard (no `E`/`R` commands; identity = BT name).
- **Home**: robot-gated tile grid.
- **PadScreen**: the 20 movements (hold-to-repeat like Zowi's pad) + speed
  selector; arms pad section when `hasArms` && fork.
- **GestureScreen**: 13 gestures.
- **MouthScreen (8x8 player)**: ~30 presets from `Otto_mouths.h`.
- **MouthEditorScreen (8x8)**: Zowi's 6x5 editor parameterized to an 8x8 grid
  (64-bit pattern, live `L` updates).
- **SoundScreen**: 19 melodies (`K`) + piano via `T` (reuses the
  `.local/EXTRA_SOUNDS.md` PianoScreen design).
- **CalibrationScreen**: trims for 4 servos (6 with arms), `C`/`G` commands.
- **SettingsScreen**: connection/transport status, detected firmware +
  capabilities, `hasMouth` / `hasArms` toggles, forget device.

## Configuration & session keys

- Session: `activeOttoDeviceAddress`, `activeOttoName`,
  `activeOttoTransport` (bt/usb/ble), plus preferences
  `ottoHasMouth`, `ottoHasArms`, `ottoFirmware` (auto/official/fork),
  `ottoBaudRate`.
- i18n contexts per QML file name, 5 locales from day one.

## Risks & open items

- Official firmware's **default handler stops the robot on any unknown
  command**: capability probes must be idle-safe (`V` only right after
  connect or when rest state is known).
- **Baud rate variance** per BT module (9600 vs 57600): expose in settings,
  auto-retry connect at both.
- **MAX7219 orientation** is a compile-time `#define` in firmware (1–4):
  document; editor preview cannot fix a mis-oriented matrix at runtime.
- `delay(30)` inside `sendAck()`/`sendFinalAck()` caps sequencer cadence in
  ACK mode; the fork should drop or shorten it.
- SoftwareSerial reliability at >9600 baud on AVR: keep 9600 as default.
- Arms protocol (moveIds 21+, pins 6/7) is our own extension: document it in
  `docs/` and keep the fork's diff minimal against `Otto_APP.ino` v13.

## Effort estimate

Roughly **1.5–2 weeks**:

| Chunk | Days |
|---|---|
| Core port (64-bit mouths, capabilities, dual sequencer mode) + tests | 1 |
| SPP + USB backend reuse/config | 0.5 |
| BLE GATT backend | 2–3 |
| Firmware fork (dual stream, arms, `V`, sensors) | 1–2 |
| GUI screens (incl. 8x8 editor/player) + i18n | 3–4 |
| Packaging, CI, docs | 1 |

## References

- `OttoDIY/OttoDIYLib` (GitHub): `examples/Otto_APP/Otto_APP.ino` (protocol
  source of truth, v13), `src/Otto_mouths.h` (8x8 presets),
  `src/Otto_gestures.h`, `src/Otto_sounds.h`, `Otto9Humanoid` (arms pins).
- OttoDIY hardware: Arduino Nano + Nano I/O shield, 4–6 servos, buzzer pin 13,
  MAX7219 8x8 matrix on DIN=A3/CS=A2/CLK=A1, HC-05/HC-06 or BLE serial module.
- ZowiDesktop repo as architectural template; `.local/MBOT_DESKTOP.md` and
  `.local/EXTRA_SOUNDS.md` for shared design decisions (BLE backend,
  PianoScreen).
