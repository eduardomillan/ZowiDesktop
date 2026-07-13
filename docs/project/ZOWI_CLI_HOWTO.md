# Zowi CLI — How To

The `zowi_cli` tool provides terminal access to Zowi Desktop's core functionality without launching the GUI.

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
| `restore` | Restore the original factory firmware |
| `alarm` | Install the Robot Alarm firmware |

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

### Avoid CAP_NET_ADMIN warning

On Linux, Bluetooth scanning requires elevated permissions. Run with `sudo` to suppress the warning:

```bash
sudo zowi_cli scan
```

Or grant the capability permanently:

```bash
sudo setcap cap_net_admin+ep build/src/cli/zowi_cli
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

### Avoid CAP_NET_ADMIN warning

```bash
sudo zowi_cli connect B4:9D:0B:32:41:0E
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

### Avoid CAP_NET_ADMIN warning

```bash
sudo zowi_cli rename "Mi Zowi"
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
active when the SPP serial connection is established — the classic Arduino
auto‑reset, but driven by the *connection* rather than a host DTR line. The
bootloader then waits only a short window (~1 s) for the first `STK_GET_SYNC`.

This is why flashing must **bind the RFCOMM TTY and open/stream it within the same
process**: if the SPP connection is established by a separate `rfcomm bind` command,
the reset fires there and the bootloader window expires long before the CLI opens the
TTY — you get zero response bytes. So always let the CLI auto‑bind (run with
`--address`, no `--tty`), as root.

Because the Qt Bluetooth SPP *socket* path cannot open a real TTY, flashing uses a
**serial (RFCOMM TTY) backend** instead:

1. The CLI binds an RFCOMM TTY to the paired device in‑process (`rfcomm bind 0 <address> 1`)
   and opens it immediately, so the connection‑triggered reset and the first
   `STK_GET_SYNC` land inside the bootloader window. (A brief DTR pulse is also sent
   for boards that wire DTR to RESET.)
2. The TTY is opened at 9600 8N1.
3. The HEX file is parsed (Intel HEX) and programmed over STK500v1:
   `STK_GET_SYNC` → `STK_ENTER_PROGMODE` → chip‑erase (universal `0xAC 0x80`) →
   for each 128‑byte page: `STK_LOAD_ADDRESS` + `STK_PROG_PAGE` (memtype `'F'`) →
   `STK_LEAVE_PROGMODE` (the bootloader then reboots into the new firmware).
4. The CLI waits for the robot to report its new app ID (`&&I <appId>%%`).

`--protocol stk` is the default. `--protocol raw` streams the HEX verbatim instead
(kept only for experimenting with non‑Optiboot bootloaders).

### Serial / TTY requirements

Flashing must open a real serial port (so the connection‑triggered reset and the
first bootloader sync happen in the same process), not a Bluetooth socket. On Linux
that means an RFCOMM TTY device such as `/dev/rfcomm0`, created with `CAP_NET_ADMIN`
(typically root).

**Do not** pre‑bind and run in two separate steps — that breaks the bootloader timing
(see *How it works* above). Instead, run the flashing command as root **without**
`--tty`; the CLI binds and opens the TTY itself, in‑process:

```bash
sudo zowi_cli restore --address B4:9D:0B:32:41:0E
sudo zowi_cli alarm  --address B4:9D:0B:32:41:0E
```

You may omit `--address` if the device was already paired (the CLI reads
`activeZowiDeviceAddress` from the session). You may pass `--tty /dev/rfcomm0` only if
that TTY was created in the *same* process right before (not via a prior `rfcomm bind`
command). The CLI releases the auto‑bound TTY afterwards.

### Basic usage

```bash
sudo zowi_cli restore --address B4:9D:0B:32:41:0E
```

Output:

```text
Connecting to B4:9D:0B:32:41:0E for firmware restore...
Serial connection open.
Bootloader mode: skipping battery check and uploading immediately.
Streaming firmware to bootloader...
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

Identical to `restore` — the CLI auto‑binds an RFCOMM TTY in‑process (so the
connection‑triggered reset and the first bootloader sync stay inside the bootloader
window) and streams the Intel‑HEX firmware file over STK500v1. The installed
`ZOWI_Alarm_v2` firmware **persists** on the robot until you run `restore`. See the
`restore` section for the full protocol description and *Serial / TTY requirements*
(flashing must run as root / with `CAP_NET_ADMIN`).

### Basic usage

```bash
zowi_cli alarm
```

Output:

```text
Connecting to B4:9D:0B:32:41:0E for Alarm firmware installation...
Connected. Checking battery level...
Battery reported: 85%
Entering bootloader...
Bootloader synchronized.
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

Shell scripts for testing CLI commands are in `src/cli/tests/`.

### test_connect_rename.sh

Scans for a Zowi, connects, checks status, renames to "TestZowi", verifies status again, and disconnects.

```bash
./src/cli/tests/test_connect_rename.sh
```

### test_disconnect.sh

Checks if a Zowi is connected and disconnects it, or reports no device.

```bash
./src/cli/tests/test_disconnect.sh
```

### test_restore_factory_firmware.sh

Runs the factory firmware restore flow against the currently paired Zowi and then shows the updated status.

```bash
./src/cli/tests/test_restore_factory_firmware.sh
```

### test_install_alarm.sh

Runs the Robot Alarm firmware install flow against the currently paired Zowi and then shows the updated status.

```bash
./src/cli/tests/test_install_alarm.sh
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
Connecting to B4:9D:0B:32:41:0E for firmware restore...
Connected. Checking battery level...
Battery reported: 85%
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
zowi_cli status --help       # Status help
zowi_cli restore --help      # Restore help
zowi_cli alarm --help        # Alarm help
```
