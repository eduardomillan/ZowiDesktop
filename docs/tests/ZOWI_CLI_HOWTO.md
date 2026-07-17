# Testing the Zowi CLI over USB

This guide lists the commands to exercise the `zowi_cli` tool over a **USB serial
link** (a machine without Bluetooth), plus the test scripts that automate them.

For the general CLI reference and the Bluetooth workflow, see
[`docs/project/ZOWI_CLI_HOWTO.md`](../project/ZOWI_CLI_HOWTO.md).

The examples assume the binary at `build/src/cli/zowi_cli` and a robot connected
over USB (e.g. `/dev/ttyUSB0`).

## Prerequisites

```bash
# Build just the CLI
./build.sh --cli

# General help; confirm the 'ports' subcommand is present
./build/src/cli/zowi_cli --help
```

### Serial port permissions

Opening a USB TTY needs access to the serial device. If you get a
"permission denied" error:

```bash
# Persistent: add your user to the dialout group (re-login required)
sudo usermod -aG dialout "$USER"

# Temporary, for the current session only:
sudo chmod a+rw /dev/ttyUSB0
```

Unlike the Bluetooth serial backend (RFCOMM), the USB backend does **not**
require `CAP_NET_ADMIN` / root — there is no `rfcomm bind` step.

## Port discovery

```bash
# Enumerate USB serial ports (/dev/ttyUSB*, /dev/ttyACM*)
./build/src/cli/zowi_cli ports
```

`ports` is the USB analogue of the Bluetooth `scan` command.

## Option validation (no robot required)

```bash
# USB options on restore/alarm
./build/src/cli/zowi_cli restore --help
./build/src/cli/zowi_cli alarm --help

# Clean failure with a non-existent port
./build/src/cli/zowi_cli restore --backend usb --tty /dev/tty_nope --baud 115200 -t 1 --force-low-battery
```

## Flashing over USB (robot connected)

```bash
# Restore factory firmware over USB, auto-selecting the first port
./build/src/cli/zowi_cli restore --backend usb --baud 115200 --force-low-battery

# Restore specifying the port explicitly
./build/src/cli/zowi_cli restore --backend usb --tty /dev/ttyUSB0 --baud 115200 --force-low-battery

# Install the alarm firmware over USB
./build/src/cli/zowi_cli alarm --backend usb --tty /dev/ttyUSB0 --baud 115200 --force-low-battery

# Custom firmware and timeout
./build/src/cli/zowi_cli restore --backend usb --tty /dev/ttyUSB0 --baud 57600 \
    -f src/firmware/ZOWI_BASE_v2.hex -t 20 --force-low-battery

# Alternative: only --tty (implies the serial backend), using the STK500v1 protocol
./build/src/cli/zowi_cli restore --tty /dev/ttyUSB0 --baud 115200 --protocol stk --force-low-battery
```

### A note on baud rate

`--baud` defaults to `9600` (the RFCOMM/ZUM BT-328 bootloader rate). Optiboot
over a USB serial link typically runs at `57600` or `115200`. If a flash fails
with a sync error, try a different `--baud`.

## Test scripts

The USB test scripts live in `src/cli/tests/usb/` (the Bluetooth tests are in
`src/cli/tests/bt/`). Set `ZOWI_CLI` to override the binary path.

### run_all.sh

Runs all USB tests in order. By default only the hardware-free smoke tests run;
set `ZOWI_USB_FLASH=1` (with a robot connected) to also run the flashing tests.

```bash
# Smoke tests only (no robot needed)
./src/cli/tests/usb/run_all.sh

# Full run, including flashing (needs a robot on USB)
ZOWI_USB_FLASH=1 ZOWI_USB_TTY=/dev/ttyUSB0 ZOWI_USB_BAUD=115200 \
    ./src/cli/tests/usb/run_all.sh
```

Environment variables:

| Variable | Purpose | Default |
|---|---|---|
| `ZOWI_CLI` | Path to the `zowi_cli` binary | `build/src/cli/zowi_cli` |
| `ZOWI_USB_FLASH` | Set to `1` to run the flashing tests | `0` |
| `ZOWI_USB_TTY` | USB serial port for flashing | auto-picked |
| `ZOWI_USB_BAUD` | Baud rate for flashing | `115200` |

### test_ports.sh

Enumerates USB serial ports with the `ports` subcommand (USB analogue of the
Bluetooth `scan` test). Passes whether or not a robot is plugged in.

```bash
./src/cli/tests/usb/test_ports.sh
```

### test_usb_options.sh

Smoke test (USB analogue of `bt/test_control.sh`): verifies that `ports` is
listed, that `restore`/`alarm` expose `--backend usb` and `--baud`, and that a
non-existent port fails cleanly.

```bash
./src/cli/tests/usb/test_usb_options.sh
```

### test_restore_factory_firmware.sh

Restores the factory firmware over USB (`--backend usb`). Requires a robot.

```bash
ZOWI_USB_TTY=/dev/ttyUSB0 ZOWI_USB_BAUD=115200 \
    ./src/cli/tests/usb/test_restore_factory_firmware.sh
```

### test_install_alarm.sh

Installs the Robot Alarm firmware over USB (`--backend usb`). Requires a robot.

```bash
ZOWI_USB_TTY=/dev/ttyUSB0 ZOWI_USB_BAUD=115200 \
    ./src/cli/tests/usb/test_install_alarm.sh
```
