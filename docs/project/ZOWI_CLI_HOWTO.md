# Zowi CLI — How To

The `zowi_cli` tool provides terminal access to Zowi Desktop's core functionality without launching the GUI.

## Table of Contents

- [Quick reference](#quick-reference)
- [Help](#help)
- [Session](#session)
- [Config](#config)
- [Translate](#translate)
- [Scan](#scan)
- [Connect](#connect)
- [Rename](#rename)
- [Restore](#restore)
- [Alarm](#alarm)
- [Adivinawi](#adivinawi)
- [Disconnect](#disconnect)
- [Status](#status)
- [Test Scripts](#test-scripts)
- [Examples](#examples)
- [Control](#control)
- [Building](#building)

## Quick reference

```bash
zowi_cli <subcommand> [options]
```

| Subcommand | Description |
|-----------|-------------|
| `session` | Manage session data (persistent key-value store) |
| `config` | Read app configuration values |
| `translate` | Translate strings using the i18n engine |
| `scan` | Scan for nearby Zowi robots via Bluetooth |
| `connect` | Connect to a Zowi and receive identification data |
| `rename` | Rename a paired Zowi robot |
| `restore` | Restore the original factory firmware/functions |
| `disconnect` | Disconnect and clear pairing data |
| `status` | Show current Zowi connection status |
| `control` | Interactive keyboard minigame to drive the robot |
| `alarm` | Install the Robot Alarm firmware |
| `adivinawi` | Install the Adivinawi game firmware |



## Help

```bash
zowi_cli --help              # General help
zowi_cli session --help      # Session subcommand help
zowi_cli config --help       # Config subcommand help
zowi_cli translate --help    # Translate help
zowi_cli scan --help         # Scan help
zowi_cli connect --help      # Connect help
zowi_cli rename --help       # Rename help
zowi_cli restore --help      # Restore help
zowi_cli disconnect --help   # Disconnect help
zowi_cli status --help      # Status help
zowi_cli control --help      # Control (minigame) help
zowi_cli alarm --help        # Alarm help
zowi_cli adivinawi --help    # Adivinawi help
```


## Session

The session store persists data between app launches. Used to track wizard completion, paired device address, etc.

### List all keys

```bash
zowi_cli session list
```

Output:

```
wizardDismissed=false
```

### Get a value

```bash
zowi_cli session get wizardDismissed
```

Output:

```
false
```

### Set a value

```bash
# Boolean
zowi_cli session set wizardDismissed true

# String
zowi_cli session set activeZowiDeviceName "My Zowi"

# Integer
zowi_cli session set someCounter 42
```

Output:

```
OK
```

Type is auto-detected: `true`/`false` → bool, numeric → int, otherwise → string.

## Config

Read-only access to `src/config.json` (image paths, URLs, etc.).

### List all config keys

```bash
zowi_cli config list
```

Output:

```
know_more=https://eduardomillan.github.io/ZowiDesktop
splash_image=qrc:/images/android/hello_image.png
start_image=qrc:/images/android/pressed_animation_sleppy_button.png
welcome_image=qrc:/images/android/welcome_image.png
```

### Get a value

```bash
zowi_cli config get know_more
```

Output:

```
https://eduardomillan.github.io/ZowiDesktop
```

## Translate

Translate source text using the custom XML-based i18n engine.

### Default locale (es_ES)

```bash
zowi_cli translate -s "Hola mundo"
```

### Specific locale

```bash
zowi_cli translate -l en_US -s "Hola mundo"
zowi_cli translate -l ca_ES -s "Hola mundo"
```

### With context

```bash
zowi_cli translate -c "WelcomeScreen.qml" -s "Start"
```

Available locales: `es_ES`, `ca_ES`, `en_US`.

## Scan

Scan for nearby Zowi robots via Bluetooth.

### Default scan (Zowi devices only, 5 seconds)

```bash
zowi_cli scan
```

### Scan with custom timeout

```bash
zowi_cli scan -t 10    # 10 seconds
zowi_cli scan -t 3     # 3 seconds
```

### Show all Bluetooth devices

```bash
zowi_cli scan --no-filter-name --no-filter-mac
```

### Avoid permission warnings

Bluetooth scanning through BlueZ D‑Bus works for normal users in most setups.
If your user lacks D‑Bus bluetooth access, add it to the `bluetooth` group:

```bash
sudo usermod -aG bluetooth $USER
# log out and back in
```

## Connect

Connect to a Zowi robot by Bluetooth address, receive its identification data (name, app ID, battery level), and save the pairing to session.

### Basic usage

```bash
zowi_cli connect B4:9D:0B:32:41:0E
```

Output:

```
Connecting to B4:9D:0B:32:41:0E...
Connected. Waiting for robot data...

  Name:    Zowi
  App ID:  1
  Battery: 85.0%
  Address: B4:9D:0B:32:41:0E

Pairing saved to session.
```

### Custom timeout

```bash
zowi_cli connect B4:9D:0B:32:41:0E -t 5    # 5 seconds
```

### Verify pairing was saved

```bash
$ zowi_cli session list
activeZowiDeviceAddress=B4:9D:0B:32:41:0E
activeZowiName=Zowi
wizardDismissed=true
```

### Avoid permission warnings

Bluetooth connection through BlueZ D‑Bus works for normal users. See the `scan` section for group requirements:

```bash
zowi_cli connect B4:9D:0B:32:41:0E
```

## Rename

Rename a paired Zowi robot. Connects to the saved device, sends the rename command, and updates the session with the new name.

### Basic usage

```bash
zowi_cli rename "Mi Zowi"
```

Output:

```
Connecting to B4:9D:0B:32:41:0E...
Connected. Sending rename command...
Robot renamed to 'Mi Zowi'.
Session updated.
```

### Verify name was updated

```bash
$ zowi_cli session get activeZowiName
Mi Zowi
```

### Avoid permission warnings

Same as `connect` — BlueZ D‑Bus works for normal users:

```bash
zowi_cli rename "Mi Zowi"
```

## Restore

Restore Zowi's original factory firmware and built-in behaviors. This is useful after loading custom firmware such as Alarm/Guardian or Rock-Paper-Scissors variants.

By default, the CLI uploads the bundled factory firmware file:

```text
src/firmware/ZOWI_BASE_v2.hex
```

### How it works

The robot is flashed with the **STK500v1 / Optiboot** bootloader protocol — the same
protocol the official Android app uses (`bq/protocol-stk-500-v1`,
`STK500v1.programUsingOptiboot(false, 128)`). It is *not* a raw HEX stream.

Zowi's bootloader runs only briefly after a hardware reset. On the ZUM BT-328 board
the reset is triggered by the **HC‑05 Bluetooth module's STATE pin**, which goes
active when the SPP connection is established — the classic Arduino
auto‑reset, but driven by the *connection* rather than a host DTR line. The
bootloader then waits only a short window (~1 s) for the first `STK_GET_SYNC`.

Two backends can perform the flash:

**BlueZ SPP (default, `--backend bluetooth`).** Uses Qt Bluetooth → BlueZ D‑Bus →
the robot's RFCOMM SPP service. The STATE‑pin reset fires as soon as the SPP
connection is established, so the first `STK_GET_SYNC` sent right after connection
arrives inside the bootloader window. **No `CAP_NET_ADMIN` / root needed.** This
backend replaces the older serial‑TTY approach and is the recommended path.

**Serial / RFCOMM TTY (fallback, `--backend serial` / `--tty`).** Binds an RFCOMM
TTY (`rfcomm bind 0 <address> 1`) and opens it in‑process. The bind itself fires
the reset, so binding and opening must happen in the same process — do *not*
pre‑bind with a separate `rfcomm bind` command. Requires `CAP_NET_ADMIN` (root or
setcap). A brief DTR pulse is also sent for boards that wire DTR→RESET.

Both backends follow the same STK500v1 flow once connected:
1. Connection opened at 9600 8N1.
2. The HEX file is parsed (Intel HEX) and programmed over STK500v1:
   `STK_GET_SYNC` → `STK_ENTER_PROGMODE` → chip‑erase (universal `0xAC 0x80`) →
   for each 128‑byte page: `STK_LOAD_ADDRESS` + `STK_PROG_PAGE` (memtype `'F'`) →
   `STK_LEAVE_PROGMODE` (the bootloader then reboots into the new firmware).
3. The CLI waits for the robot to report its new app ID (`&&I <appId>%%`).

`--protocol stk` is the default. `--protocol raw` streams the HEX verbatim instead
(kept only for experimenting with non‑Optiboot bootloaders).

### Backend selection

| `--backend` | Backend | Needs root / setcap? |
|---|---|---|
| `auto` (default) | Qt Bluetooth SPP (BlueZ) | No |
| `bluetooth` | Qt Bluetooth SPP (BlueZ) | No |
| `serial` | Serial / RFCOMM TTY (`rfcomm bind`) | Yes (`CAP_NET_ADMIN`) |
| `usb` | USB serial (`/dev/ttyUSB*`, `/dev/ttyACM*`) | No |

When `--tty` is given, the serial backend is selected automatically regardless of
`--backend`.

The `usb` backend talks to the robot over a plain USB serial link (no Bluetooth,
no `rfcomm bind`), for machines without a Bluetooth adapter. Use `ports` to list
available serial ports and `--baud` to set the line speed (USB Optiboot is
typically 57600 or 115200). See
[`docs/tests/ZOWI_CLI_HOWTO.md`](../tests/ZOWI_CLI_HOWTO.md) for the full USB
workflow and test scripts.

If using the serial backend, the TTY must be bound and opened in the **same process**
— do **not** pre‑bind with a separate `rfcomm bind` command, because the bind fires
the bootloader reset and the ~1 s window expires before the CLI opens the TTY.

### Privileges: running as your normal user

The **default BlueZ SPP backend does not need any extra privileges**. Simply run:

```bash
./build/src/cli/zowi_cli restore --address B4:9D:0B:32:41:0E
./build/src/cli/zowi_cli alarm  --address B4:9D:0B:32:41:0E
./build/src/cli/zowi_cli status
```

All commands share the same user session, so `status` always shows current data.

If you use the serial backend (`--backend serial` or `--tty`), you still need
`CAP_NET_ADMIN`. Grant it with setcap:

```bash
sudo setcap cap_net_admin+ep build/src/cli/zowi_cli
```

The build applies the capability automatically on every (re)compile via a
post‑build step in `src/cli/CMakeLists.txt` (using `sudo -n`, so it only succeeds
when you already have cached/non‑interactive sudo rights). If it can't, the build
prints a reminder instead of failing.

For fully passwordless builds, allow `setcap` without a password:

```bash
sudo visudo   # add:  youruser ALL=(root) NOPASSWD: /sbin/setcap
```

### Alternative: run as root (serial backend only)

If you use `--backend serial` (or `--tty`) and prefer not to set the capability,
run the flashing command as root **without** `--tty`; the CLI binds and opens the
TTY itself, in‑process. Remember to run `status`/`connect` as root too, or they
will read a different (user) session:

```bash
sudo zowi_cli restore --backend serial --address B4:9D:0B:32:41:0E
sudo zowi_cli alarm  --backend serial --address B4:9D:0B:32:41:0E
```

You may omit `--address` if the device was already paired. You may pass
`--tty /dev/rfcomm0` only if that TTY was created in the *same* process right
before (not via a prior `rfcomm bind`). The CLI releases the auto‑bound TTY
afterwards.

### Basic usage

```bash
./build/src/cli/zowi_cli restore --address B4:9D:0B:32:41:0E
```

Output:

```text
Connecting to B4:9D:0B:32:41:0E...
Connection open.
Bootloader mode: skipping battery check and uploading immediately.
Uploading firmware from src/firmware/ZOWI_BASE_v2.hex...
  Progress: 100%
Waiting for the restored firmware to report its app ID...
Factory firmware restored.
  App ID:  ZOWI_BASE_v2
Session updated.
```

If the bootloader cannot be reached (e.g. the TTY was not reset into the bootloader),
double-check that the RFCOMM TTY was created and that the robot was reachable.

### Custom firmware path

```bash
zowi_cli restore -f /path/to/ZOWI_BASE_v2.hex
```

### Low battery handling

The restore flow follows the Android app's battery warning threshold of **50%**.

If the robot reports less than 50% battery, the command stops unless you explicitly continue:

```bash
zowi_cli restore --force-low-battery
```

### Custom timeout

```bash
zowi_cli restore -t 15
```

## Alarm

Install the Robot Alarm firmware (project "Robot Alarma") on the paired Zowi. This is one of the custom firmware variants that the factory restore can revert.

By default, the CLI uploads the bundled alarm firmware file:

```text
src/firmware/ZOWI_Alarm_v2.hex
```

### How it works

Identical to `restore` — the CLI connects via the default BlueZ SPP backend
(sudo‑free) and streams the Intel‑HEX firmware over STK500v1. The installed
`ZOWI_Alarm_v2` firmware **persists** on the robot until you run `restore`. See the
`restore` section for the full protocol description, backend selection, and privilege
requirements.

### Basic usage

```bash
./build/src/cli/zowi_cli alarm --address B4:9D:0B:32:41:0E
```

Output:

```text
Connecting to B4:9D:0B:32:41:0E...
Connection open.
Bootloader mode: skipping battery check and uploading immediately.
Uploading firmware from src/firmware/ZOWI_Alarm_v2.hex...
  Progress: 100%
Waiting for the updated firmware to report its app ID...
Alarm firmware installed.
  App ID:  ZOWI_Alarm_v2
Session updated.
```

### Custom firmware path

```bash
zowi_cli alarm -f /path/to/ZOWI_Alarm_v2.hex
```

### Low battery handling

The alarm flow follows the same 50% battery warning threshold as restore:

```bash
zowi_cli alarm --force-low-battery
```

### Custom timeout

```bash
zowi_cli alarm -t 15
```

## Adivinawi

Install the Adivinawi game firmware (project "Adivinawi") on the paired Zowi. Like Alarm, this is one of the custom firmware variants that the factory restore can revert.

By default, the CLI uploads the bundled Adivinawi firmware file:

```text
src/firmware/ZOWI_Adivinawi_v2.hex
```

### How it works

Identical to `alarm`/`restore` — the CLI connects via the default BlueZ SPP backend
(sudo‑free) and streams the Intel‑HEX firmware over STK500v1. The installed
`ZOWI_Adivinawi_v2` firmware **persists** on the robot until you run `restore`. See the
`restore` section for the full protocol description, backend selection, and privilege
requirements.

### Basic usage

```bash
./build/src/cli/zowi_cli adivinawi --address B4:9D:0B:32:41:0E
```

Output:

```text
Connecting to B4:9D:0B:32:41:0E...
Connection open.
Bootloader mode: skipping battery check and uploading immediately.
Uploading firmware from src/firmware/ZOWI_Adivinawi_v2.hex...
  Progress: 100%
Waiting for the updated firmware to report its app ID...
Adivinawi firmware installed.
  App ID:  ZOWI_Adivinawi_v2
Session updated.
```

### Custom firmware path

```bash
zowi_cli adivinawi -f /path/to/ZOWI_Adivinawi_v2.hex
```

### Low battery handling

The Adivinawi flow follows the same 50% battery warning threshold as restore:

```bash
zowi_cli adivinawi --force-low-battery
```

### Custom timeout

```bash
zowi_cli adivinawi -t 15
```

### USB

Like Alarm, Adivinawi can be flashed over a USB serial link:

```bash
zowi_cli ports
zowi_cli adivinawi --backend usb --tty /dev/ttyUSB0 --baud 115200
```

## Disconnect

Clear all pairing data from the session store.

### Basic usage

```bash
zowi_cli disconnect
```

Output:

```
Disconnected from Mi Zowi [B4:9D:0B:32:41:0E]
Pairing data cleared.
```

### Verify pairing was cleared

```bash
$ zowi_cli session list
wizardDismissed=false
```

## Status

Show the current Zowi connection status. `status` opens a **live** Bluetooth
connection to the paired robot and reports its real running firmware (name, app ID,
battery), refreshing the session cache. If the robot cannot be reached it falls back
to the last known (cached) values and marks them `(cached)`.

> Note: run `status` as the **same user** that ran `connect`/`restore`/`alarm`. Those
> flashing commands need `CAP_NET_ADMIN`, so either run them with `sudo` (which writes
> the *root* session) or grant the binary the capability and run everything as your
> user — see *Serial / TTY requirements*.

### Basic usage

```bash
zowi_cli status
```

Output when connected:

```
Zowi connected:
  Name:    Mi Zowi
  Address: B4:9D:0B:32:41:0E
  App ID:  1
  Battery: 85%
  Wizard:  completed
```

Output when no device is paired:

```
No Zowi connected.
```

## Test Scripts

Shell scripts for testing CLI commands are grouped by transport:
`src/cli/tests/bt/` (Bluetooth) and `src/cli/tests/usb/` (USB). Each folder has a
`run_all.sh` that runs its tests in order. The USB scripts and workflow are
documented in [`docs/tests/ZOWI_CLI_HOWTO.md`](../tests/ZOWI_CLI_HOWTO.md).

### bt/run_all.sh

Runs the Bluetooth tests in order. By default only tests that degrade gracefully
without a robot run; set `ZOWI_BT_FULL=1` (with a robot in range) to run the
connect/rename, control and flashing tests too.

```bash
./src/cli/tests/bt/run_all.sh
ZOWI_BT_FULL=1 ./src/cli/tests/bt/run_all.sh
```

### bt/test_connect_rename.sh

Scans for a Zowi, connects, checks status, renames to "TestZowi", verifies status again, and disconnects.

```bash
./src/cli/tests/bt/test_connect_rename.sh
```

### bt/test_disconnect.sh

Checks if a Zowi is connected and disconnects it, or reports no device.

```bash
./src/cli/tests/bt/test_disconnect.sh
```

### bt/test_restore_factory_firmware.sh

Runs the factory firmware restore flow against the currently paired Zowi and then shows the updated status.

```bash
./src/cli/tests/bt/test_restore_factory_firmware.sh
```

### bt/test_install_alarm.sh

Runs the Robot Alarm firmware install flow against the currently paired Zowi and then shows the updated status.

```bash
./src/cli/tests/bt/test_install_alarm.sh
```

### bt/test_install_adivinawi.sh

Runs the Adivinawi game firmware install flow against the currently paired Zowi and then shows the updated status.

```bash
./src/cli/tests/bt/test_install_adivinawi.sh
```

## Examples

### Full pairing workflow

```bash
# 1. Scan for nearby robots
$ zowi_cli scan
Scanning for 5s...
Zowi [B4:9D:0B:32:41:0E]
1 device(s) found.

# 2. Connect and pair
$ zowi_cli connect B4:9D:0B:32:41:0E
Connecting to B4:9D:0B:32:41:0E...
Connected. Waiting for robot data...

  Name:    Zowi
  App ID:  1
  Battery: 85.0%
  Address: B4:9D:0B:32:41:0E

Pairing saved to session.

# 3. Rename the robot
$ zowi_cli rename "Mi Zowi"
Connecting to B4:9D:0B:32:41:0E...
Connected. Sending rename command...
Robot renamed to 'Mi Zowi'.
Session updated.

# 4. Restore the original firmware if needed
$ zowi_cli restore
Connecting to B4:9D:0B:32:41:0E...
Connection open.
Bootloader mode: skipping battery check and uploading immediately.
Uploading firmware from src/firmware/ZOWI_BASE_v2.hex...
  Progress: 100%
Waiting for the restored firmware to report its app ID...
Factory firmware restored.
  App ID:  ZOWI_BASE_v2
Session updated.

# 5. Verify
$ zowi_cli session list
activeZowiDeviceAddress=B4:9D:0B:32:41:0E
activeZowiName=Mi Zowi
wizardDismissed=true

# 6. Disconnect when done
$ zowi_cli disconnect
Disconnected from Mi Zowi [B4:9D:0B:32:41:0E]
Pairing data cleared.
```

### Check if wizard was completed

```bash
$ zowi_cli session get wizardDismissed
false
```

### Reset wizard state

```bash
$ zowi_cli session set wizardDismissed false
OK
```

### Find Zowi and show its address

```bash
$ zowi_cli scan -t 5
Scanning for 5s...
Zowi [B4:9D:0B:32:41:0E]
1 device(s) found.
```

### Debug translations

```bash
$ zowi_cli translate -l es_ES -s "Welcome"
Bienvenido

$ zowi_cli translate -l en_US -s "Welcome"
Welcome
```




## Control

Interactive keyboard minigame that drives the Zowi robot in real time. Connects
to the paired robot (or to an explicit `--address`) and reads the cursor keys
from the terminal, sending one movement command per key press. Movement
commands follow the firmware serial protocol documented in
`docs/firmware/PROTOCOL.md` (the `M <MoveID> <T> [<MoveSize>]` movement command and the `S`
stop command).

Supports both Bluetooth (default) and USB serial transport. Use `--backend usb`
to drive the robot over a USB cable instead of Bluetooth.

### Backend selection

| `--backend` | Backend | Needs root / setcap? |
|---|---|---|
| `auto` (default) | Uses the transport registered at `connect` time; falls back to Bluetooth | No |
| `bluetooth` | Qt Bluetooth SPP (BlueZ) | No |
| `usb` | USB serial (`/dev/ttyUSB*`, `/dev/ttyACM*`) | No |

When `--tty` is given, the USB serial backend is selected automatically
regardless of `--backend`.

### Controls

| Key            | Action              | Firmware command           |
|----------------|---------------------|----------------------------|
| `↑` / `W`      | Walk forward        | `M 1 <T>`                  |
| `↓` / `S`      | Walk backward       | `M 2 <T>`                  |
| `←` / `A`      | Moonwalker left     | `M 6 <T> 30`               |
| `→` / `D`      | Moonwalker right    | `M 7 <T> 30`               |
| `Q`            | Turn left           | `M 3 <T>`                  |
| `E`            | Turn right          | `M 4 <T>`                  |
| `+`            | Increase speed      | (changes `T` for next move)|
| `-`            | Decrease speed      | (changes `T` for next move)|
| `ESC` / `Ctrl-C` | Quit              | `S` (stop) on exit         |

Both the cursor keys and the WASD/Q/E letter keys are supported. The terminal is
switched to raw mode while the minigame runs, so keys are delivered immediately
(no Enter needed) and are not echoed. The original terminal settings are
restored on exit (including on `Ctrl-C`).

When a movement key is pressed, the terminal displays the uppercase key token
and the action, e.g. `[UP] forward` or `[LEFT] moonwalker left`. When the speed
is changed, `[SPEED: SLOW]` / `[SPEED: MEDIUM]` / `[SPEED: FAST]` is shown.

After 1 second of inactivity, the robot stops and the terminal shows:
`Status: idle. Speed: MEDIUM. Last key: UP (forward)`.

### Basic usage (paired device)

```bash
zowi_cli control
```

### Connect to a specific robot

```bash
zowi_cli control --address B4:9D:0B:32:41:0E
```

### Choose a movement speed

```bash
zowi_cli control --speed slow     # also: medium (default), fast
```

Speed maps to the firmware period `T` in ms: `slow` = 2000, `medium` = 1000,
`fast` = 700 (larger = slower gait).

### Custom connection timeout

```bash
zowi_cli control -t 5    # wait up to 5 seconds for the connection
```

### Drive over USB

```bash
zowi_cli control --backend usb --tty /dev/ttyUSB0
```

Or let the CLI auto-detect the USB port:

```bash
zowi_cli control --backend usb
```

When using USB, the connection timeout is automatically extended to at least 8
seconds to account for the robot's boot delay over serial. The baud rate
defaults to 115200 (the control firmware's rate); override with `--baud` if
needed.

### Behavior notes

- Each key press sends a single gait cycle; hold the key (OS auto-repeat) to
  keep moving.
- `+` and `-` cycle the speed (slow → medium → fast / fast → medium → slow).
  The `--speed` option sets the initial speed; changes during the session
  persist until the next speed change.
- After 1 second of inactivity, a stop command (`S`) is sent automatically.
- On exit the robot receives a stop command (`S`) and the terminal is restored.
- If the battery is below 50% a warning is printed (movement is still allowed).
- If stdin is not a terminal, the minigame refuses to start (it needs the
  keyboard) and exits without driving the robot.

## Building

```bash
# Build CLI only (fast, no Qt GUI needed)
./build.sh --cli

# Build everything (GUI + CLI)
./build.sh

# Build CLI with Qt 5
./build.sh -5 --cli
```

The binary is at `build/src/cli/zowi_cli`.