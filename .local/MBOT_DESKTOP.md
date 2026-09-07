# MBotDesktop — Feasibility & v1 Plan

Companion app to ZowiDesktop for Makeblock **mBot v1.1** robots. Decided:
**new standalone repo `MBotDesktop`** (skeleton cloned from ZowiDesktop) and
**USB/2.4G + BLE transports from v1**.

## Table of contents

- [Feasibility summary](#feasibility-summary)
- [What is reused from ZowiDesktop](#what-is-reused-from-zowidesktop)
- [What is new (the real work)](#what-is-new-the-real-work)
- [Core (Qt-free, `mbot::` namespace)](#core-qt-free-mbot-namespace)
- [Backends](#backends)
- [GUI v1 screens](#gui-v1-screens)
- [Firmware requirements](#firmware-requirements)
- [Risks & open items](#risks--open-items)
- [Effort estimate](#effort-estimate)
- [References](#references)

---

## Feasibility summary

Yes, feasible. The mBot speaks the open, documented **Makeblock serial
protocol** (binary `FF 55` frames), its factory firmware source is public
(mirrorable/verifiable exactly like ZowiDesktop does with
`ZOWI_BASE_v2.ino`), and ZowiDesktop's architecture (Qt-free core + transport
backends + QML adapter) is robot-agnostic except for the protocol layer.

## What is reused from ZowiDesktop

- `src/backends/bt_serial` (`SerialBluetoothBackend`, QSerialPort with
  configurable baud) as-is for **USB** (mCore CDC serial @ 115200) and the
  **2.4G wireless serial dongle**.
- Qt-free core patterns: situation/state machine, session & config stores,
  `TranslationEngine` + i18n layout (5 locales), `ctest` conventions.
- QML shell: `ScreenTemplate`, StackView navigation in `main.qml`,
  `StatusBar`/`MessageBar`, config.json pattern.
- Build/packaging scaffolding: `build.sh`, `VERSION` single-source pattern,
  `.deb`/AppImage scripts, CI workflow layout.
- Methodology: mirror the on-robot firmware parser in core and keep it in
  sync with a verify script (like `scripts/verify_arduino_mirrors.sh`).

## What is new (the real work)

- **Binary Makeblock codec** (vs. Zowi's ASCII commands): frames
  `FF 55 <len> <idx> <action> <device> [payload]`, action `0x02` = run,
  `0x01` = get. Device IDs: MOTOR `0x0A` (ports M1=9, M2=10), JOYSTICK `0x05`
  (differential drive: left+right speed in one frame), RGBLED `0x08` (2
  onboard LEDs, position 1=left/2=right/0=both), TONE `0x22` (buzzer
  freq+duration); gets: ULTRASONIC `0x01`, LIGHT `0x03`, LINEFOLLOWER `0x11`,
  BUTTON `0x16`.
- **Responses**: `FF 55 <len> <idx> <4-byte float> 0D 0A`; request/response
  matched by the `idx` byte (round-robin sensor polling needs outstanding
  request tracking).
- **BLE backend**: the mBot 1.1 Bluetooth module is **BLE**, not classic SPP
  like Zowi → new backend on `QLowEnergyController` (GATT serial service,
  write + notify, OS-level bonding, reconnect).
- **Different screen set**: no LED matrix/mouths, no servos/calibration, no
  registration wizard; instead differential drive pad, RGB color picker,
  buzzer/piano, sensor telemetry.

## Core (Qt-free, `mbot::` namespace)

- `protocol.h/.cpp`: frame builders `run(device, port, payload)` and
  `get(device, port, idx)`; motor helpers (dual MOTOR or single JOYSTICK
  frame), RGB set, tone, sensor getters.
- Stream parser/framer: detect `FF 55`, read `len`, consume until `0D 0A`,
  decode the 4-byte float, dispatch by `idx`. Byte order and field layout to
  be confirmed against `mbot_factory_firmware.ino` (`writeHead()` /
  `writeSerial(idx)` / `sendFloat()`) and on-hardware tests.
- Situation state machine (simplified vs. Zowi): Connected / Disconnected /
  TransportLost + active transport (bt/usb). Single active transport at a
  time: the mCore serial line is shared by the BT module and USB.
- Stores: session (`activeMbotDeviceAddress`, `activeMbotName`,
  `activeMbotTransport`), config, translations — copied and trimmed from
  Zowi core.
- Tests: `ctest` suites for codec and parser (mirroring
  `test_robot_commands.cpp` / `test_message_parser` style).

## Backends

- `serial`: copy of Zowi's `bt_serial` backend (QSerialPort, 115200) for USB
  and 2.4G dongle.
- `ble` (new): `QLowEnergyController`; scan by device name
  ("Makeblock"/"mBot"), discover the serial-like GATT service, write
  (with/without response per characteristic properties) + notifications as
  the RX path; reconnect loop like the Zowi BT backend. Exact service/
  characteristic UUIDs to be dumped from the real robot during the first
  hardware session (make them configurable constants until confirmed).
- Shared abstract interface copied from Zowi's `bluetooth_api.h` (renamed),
  so the GUI/`RobotController`-equivalent stays transport-agnostic.

## GUI v1 screens

- **Splash** → **Welcome/Scan**: USB port detection + BLE scan, connect.
- **Home**: tile grid (robot-gated like Zowi's HomeScreen).
- **PadScreen**: differential drive hold-buttons (forward, backward, turn
  left/right) via JOYSTICK/MOTOR frames; stop on release.
- **RgbLedScreen**: color picker applied to left / right / both onboard LEDs.
- **SoundScreen / PianoScreen**: buzzer tones via TONE frames (reuses the
  PianoScreen design from `.local/EXTRA_SOUNDS.md`; mBot has no pre-stored
  melodies, so piano/notes only).
- **SensorsScreen**: live telemetry — ultrasonic distance (cm), onboard
  light levels, line-follower state, button pressed; round-robin `get`
  polling with `idx` matching.
- **SettingsScreen**: connection status, transport info, forget device.
- No rename wizard (BLE/USB name is fixed), no calibration screen.
- i18n contexts per QML file name, 5 locales from day one.

## Firmware requirements

**v1 assumes the factory firmware** (V1.20101, the version the public serial
protocol doc describes) or any firmware implementing the same Makeblock
`FF 55` protocol. With it, every building block the games and basic features
need is already exposed over serial: motors (differential drive), RGB LEDs,
buzzer tones, ultrasonic, light sensors, line follower, button and IR. All
game logic lives in the desktop app (send commands / poll sensors), exactly
like ZowiDesktop does with Zowi — no on-robot code required.

- **mBlock-uploaded programs break the protocol**: if the mBot runs a user
  program flashed from mBlock instead of the factory firmware, the serial
  protocol handlers are gone. The app should detect this (no answers to
  `get` requests) and guide the user to restore the factory firmware via
  mBlock ("Upgrade Firmware") or the Arduino IDE. A ZowiDesktop-style
  "firmware restore" feature in MBotDesktop is a possible later addition,
  not a v1 requirement.
- **Custom firmware is optional, not a prerequisite**: flashing our own
  `.ino` on the mCore (Arduino-compatible) would only be needed for
  on-board logic — precise reaction timing, PC-free autonomous modes,
  per-action ACKs, stored melodies. That is a v2-style extension (the Zowi
  approach), never a v1 dependency.
- **On-board modes do not interfere**: the factory firmware's button-cycled
  modes (line follow / obstacle / manual) yield to remote control on the
  first serial command (`controlflag = BLUE_TOOTH` in the factory source).

## Risks & open items

- **BLE GATT UUIDs** of the Makeblock module: confirm by dumping services
  from the real mBot; keep as configurable constants until then.
- **Float byte order / response layout**: verify against factory firmware
  source and hardware loopback tests before trusting telemetry.
- **Default ports** in factory firmware (ultrasonic / line follower /
  onboard light sensors): pin down from `mbot_factory_firmware.ino`.
- **Shared serial line**: BT module and USB contend for the mCore serial —
  enforce one active transport (situation machine already models this).
- **Tone blocking behavior**: factory firmware `buzzer.tone(hz, ms)` timing
  semantics (blocking vs. timer-based) affect piano playability; measure on
  hardware.
- Firmware version drift: target factory firmware V1.20101 (the version the
  public protocol doc describes); document supported versions.

## Effort estimate

Roughly **2 weeks** part-time-equivalent:

| Chunk | Days |
|---|---|
| Core codec + parser + tests | 1–2 |
| Serial backend port | 0.5 |
| BLE backend (scan/connect/GATT/reconnect) | 2–3 |
| GUI screens + i18n | 3–4 |
| Packaging, CI, docs | 1–2 |

## References

- Makeblock "mBot Serial Port Protocol" (frame examples for motor, buzzer,
  RGB LED, ultrasonic/light/line-follower gets).
- `Makeblock-official/Makeblock-Libraries` →
  `examples/Firmware_For_mBlock/mbot_factory_firmware/mbot_factory_firmware.ino`
  (device IDs, `runModule()`/`readSensor()` parsers, port defaults).
- ZowiDesktop repo: `src/backends/bt_serial`, `src/core/`, `src/views/` as
  the architectural template; `.local/EXTRA_SOUNDS.md` (PianoScreen design).
