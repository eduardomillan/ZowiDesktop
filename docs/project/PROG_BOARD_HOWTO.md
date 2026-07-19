# Programming the Zowi Board (Firmware Upload)

## Overview

ZowiDesktop **does not use PlatformIO** or an external `avrdude`. It is a
cross-platform desktop application built with **Qt/QML and C++ (CMake)** that
flashes firmware to the robot by implementing the bootloader protocol itself.

## Firmware Format

Firmware images are shipped as **Intel HEX** files, bundled under
`src/firmware/`:

- `ZOWI_BASE_v2.hex`
- `ZOWI_Alarm_v2.hex`

## Upload Protocol

The upload logic is a self-contained implementation of the
**STK500v1 / Optiboot** bootloader protocol (the same family used by
Arduino / ATmega boards):

- Header: `src/firmware/include/zowi/stk500v1.h`
- Implementation: `src/firmware/src/stk500v1.cpp`

## Transport

Flashing runs over **Bluetooth**, not the USB serial port used by PlatformIO.
The byte I/O is provided by the Bluetooth backends in `src/backends/`:

- `bt_qt` — Qt Bluetooth backend
- `bt_serial` — serial Bluetooth backend

The protocol code stays transport-agnostic through the `BootloaderTransport`
abstraction, which the host (CLI / GUI) fills in with three callbacks:

- `send` — send raw bytes to the device.
- `receive` — read available bytes from the device.
- `pump` — pump the host event loop so asynchronous I/O (e.g. Qt signals) can
  run while the protocol blocks waiting for a reply.

## Upload Modes

There are two ways to flash the board (`src/firmware/include/zowi/stk500v1.h`):

1. **`stk500UploadFirmware`** — standard STK500v1 framing:
   soft-reset into bootloader → `STK_GET_SYNC` → `STK_ENTER_PROGMODE` →
   for each 128-byte flash page `STK_LOAD_ADDRESS` + `STK_PROG_PAGE` →
   `STK_LEAVE_PROGMODE` → soft-reset into the new firmware.

2. **`zowiRawHexUploadFirmware`** — streams the raw Intel HEX bytes to the
   device's bootloader (the same stream the original Zowi Android app sent),
   for firmwares whose custom bootloader expects the raw HEX instead of the
   STK500v1 framing. The soft-reset sequence is sent first to reboot the
   running firmware into its bootloader.

## Summary

The flashing behaviour is equivalent to what `avrdude` / the Arduino IDE do
(STK500v1 over Intel HEX), but it is embedded directly in the application and
runs over Bluetooth, instead of relying on PlatformIO or an external `avrdude`.

## Working Over USB (Machines Without Bluetooth)

On computers that have no Bluetooth adapter it is possible to talk to the robot
over a **USB serial port** (e.g. `/dev/ttyUSB0`, `/dev/ttyACM0`). The
architecture already makes this straightforward for the following reasons:

1. **A transport abstraction already exists.** All the higher-level logic
   (robot commands and the STK500v1 protocol) works against the `BluetoothApi`
   interface (`src/core/include/zowi/bluetooth_api.h`), not against Bluetooth
   directly. Any backend implementing `connect` / `send` / `isConnected` /
   `onDataReceived` is usable.

2. **The serial backend is already generic.** `SerialBluetoothBackend`
   (`src/backends/bt_serial/`) does not actually speak Bluetooth: it opens a TTY
   with `::open(ttyPath, ...)` configured as 8N1
   (`serial_bluetooth_backend.cpp`), and even performs the DTR auto-reset
   (`pulseReset`) exactly like programming an Arduino over USB/UART. Despite its
   name, it is functionally a plain serial backend.

3. **The only Bluetooth-specific step is device selection.** In `src/cli/main.cpp`
   the code runs `rfcomm bind` to create `/dev/rfcomm0` and then hands that TTY
   to the serial backend. Pointing it at `/dev/ttyUSB0` instead makes the same
   backend work with no change to the protocol logic.

### What is needed for full USB support

- **Direct TTY selection.** The CLI already accepts `--tty`, so connecting over
  USB via `--tty /dev/ttyUSB0` (skipping `rfcomm bind`) essentially works today.
- **Configurable baud rate.** The rate is currently hard-coded to `B9600`
  (`kBootBaud` in `serial_bluetooth_backend.cpp`). Optiboot over USB typically
  runs at 57600/115200, so this should be made configurable.
- **Serial port enumeration.** Add discovery of serial ports (via
  `QSerialPortInfo`, or by listing `/dev/ttyUSB*` and `/dev/ttyACM*`) so users
  are not forced through Bluetooth pairing.
- **GUI exposure.** `RobotController` always creates a `QtBluetoothBackend`;
  the GUI needs a way to select the serial/USB backend and list available ports.

### Summary

No protocol or communication code needs to be rewritten — the backend
architecture already supports it. The work is mainly (1) making the baud rate
configurable, (2) adding serial-port enumeration, and (3) exposing the option in
the GUI. From the command line, connecting over USB with `--tty /dev/ttyUSB0` is
almost immediate.
