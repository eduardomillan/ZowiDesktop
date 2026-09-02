# Zowi CLI — How To

The `zowi_cli` tool provides terminal access to Zowi Desktop's core functionality without launching the GUI.


## Table of Contents

- [Version](#version)
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
- [Control](#control)
- [Move](#move)
- [Gesture](#gesture)
- [Mouth](#mouth)
- [Sing](#sing)
- [Calibrate](#calibrate)
- [Test Scripts](#test-scripts)
- [Examples](#examples)
- [Building](#building)


## Quick reference

```bash
zowi_cli <subcommand> [options]
```

| Subcommand | Description |
|-----------|-------------|
| `--version` | Print CLI version and exit |
| `session` | Manage session data (persistent key-value store) |
| `config` | Read app configuration values |
| `translate` | Translate strings using the i18n engine |
| `scan` | Scan for nearby Zowi robots via Bluetooth |
| `connect` | Connect to a Zowi and receive identification data |
| `rename` | Rename a paired Zowi robot |
| `restore` | Restore the original factory firmware/functions |
| `disconnect` | Disconnect and clear pairing data |
| `status` | Show current Zowi connection status |
| `calibrate` | Calibrate the 4 servo trims (interactive wizard or direct) |
| `control` | Interactive keyboard minigame to drive the robot |
| `move` | Send a single movement command (forward, backward, turn, etc.) |
| `gesture` | Play a gesture animation (happy, sad, victory, etc.) |
| `mouth` | Display a mouth/LED pattern (smile, heart, etc.) |
| `sing` | Play a melody/sound (happy, connection, fart, etc.) |
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
zowi_cli move --help         # Move help
zowi_cli gesture --help      # Gesture help
zowi_cli mouth --help        # Mouth help
zowi_cli sing --help         # Sing help
zowi_cli alarm --help        # Alarm help
zowi_cli adivinawi --help    # Adivinawi help
```

## Version

Print the CLI version and exit. The version is read from `CMakeLists.txt` at build time.

```bash
zowi_cli --version
```

Output:

```
0.6.0
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

### Clear all session data

```bash
zowi_cli session clear
```

Output:

```
Session cleared.
```

Verify no keys remain:

```bash
$ zowi_cli session list
# (no output)
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

## Move

Send a single movement command to the robot and exit. Unlike `control` (which is
interactive), `move` sends one command and disconnects — useful for scripting
and automation.

### Directions

| Direction | Firmware command | Description |
|-----------|------------------|-------------|
| `forward` | `M 1 <T>` | Walk forward one gait cycle |
| `backward` | `M 2 <T>` | Walk backward one gait cycle |
| `left` | `M 3 <T>` | Turn left one gait cycle |
| `right` | `M 4 <T>` | Turn right one gait cycle |
| `moonwalker-left` | `M 6 <T> 30` | Moonwalker dance left |
| `moonwalker-right` | `M 7 <T> 30` | Moonwalker dance right |

### Speed

The `--speed` option controls the gait period `T` in milliseconds (larger = slower):

| Speed | Period (ms) |
|-------|-------------|
| `slow` | 2000 |
| `medium` (default) | 1000 |
| `fast` | 700 |

### List available movements

```bash
zowi_cli move --list
```

Output:

```
Available movements:
  forward, backward, left, right, moonwalker-left, moonwalker-right
```

### Basic usage

```bash
zowi_cli move forward
zowi_cli move backward --speed fast
zowi_cli move moonwalker-left --speed slow
```

### Options

| Option | Description |
|--------|-------------|
| `direction` | Movement direction (positional argument) |
| `-s, --speed` | Movement speed: `slow`, `medium` (default), `fast` |
| `-a, --address` | Robot Bluetooth address (overrides paired device) |
| `-t, --timeout` | Timeout waiting for connection (seconds, default `3`) |
| `--backend` | `auto` (registered transport), `bluetooth`, or `usb` |
| `--tty` | Serial TTY for USB (e.g. `/dev/ttyUSB0`) |
| `--baud` | Serial baud rate (default `115200`) |
| `-l, --list` | List available movements and exit |

### Notes

- Each invocation sends a single gait cycle. To keep moving, call `move`
  repeatedly or use `control` for interactive driving.
- The robot stops automatically after completing the movement (no explicit
  stop command needed).
- If the battery is below 50% a warning is printed (movement is still allowed).

## Gesture

Play a gesture animation on the robot. Gestures combine servo movements with
mouth expressions and sounds to convey emotions.

### Available gestures

| ID | Name | Description |
|----|------|-------------|
| 1 | `happy` | Happy expression |
| 2 | `super-happy` | Very happy expression |
| 3 | `sad` | Sad expression |
| 4 | `sleeping` | Sleeping expression |
| 5 | `fart` | Fart joke |
| 6 | `confused` | Confused expression |
| 7 | `love` | Love/heart expression |
| 8 | `angry` | Angry expression |
| 9 | `fretful` | Fretful/nervous expression |
| 10 | `magic` | Magic/wizard expression |
| 11 | `wave` | Waving gesture |
| 12 | `victory` | Victory/celebration |
| 13 | `fail` | Failure/sad outcome |

### List available gestures

```bash
zowi_cli gesture --list
```

Output:

```
Available gestures (1-13):
  1: happy        2: super-happy   3: sad          4: sleeping
  5: fart         6: confused      7: love         8: angry
  9: fretful     10: magic        11: wave        12: victory
 13: fail
```

### Basic usage

```bash
zowi_cli gesture victory
zowi_cli gesture happy
zowi_cli gesture 12          # same as victory (by ID)
```

### Options

| Option | Description |
|--------|-------------|
| `gesture` | Gesture name or ID (1-13) (positional argument) |
| `-a, --address` | Robot Bluetooth address (overrides paired device) |
| `-t, --timeout` | Timeout waiting for connection (seconds, default `3`) |
| `--backend` | `auto` (registered transport), `bluetooth`, or `usb` |
| `--tty` | Serial TTY for USB (e.g. `/dev/ttyUSB0`) |
| `--baud` | Serial baud rate (default `115200`) |
| `-l, --list` | List available gestures and exit |

### Notes

- Gestures are blocking: the robot performs the full animation before accepting
  the next command.
- You can specify the gesture by name (case-insensitive) or by numeric ID.
- The firmware protocol uses 1-based IDs (1-13); the CLI accepts both names and
  IDs.

## Mouth

Display a mouth/LED pattern on the robot's mouth matrix. The mouth is a 5x3 LED
matrix that can display simple icons and expressions.

### Available mouths

| ID | Name | Description |
|----|------|-------------|
| 0 | `zero` | Number 0 |
| 1 | `one` | Number 1 |
| 2 | `two` | Number 2 |
| 3 | `three` | Number 3 |
| 4 | `four` | Number 4 |
| 5 | `five` | Number 5 |
| 6 | `six` | Number 6 |
| 7 | `seven` | Number 7 |
| 8 | `eight` | Number 8 |
| 9 | `nine` | Number 9 |
| 10 | `smile` | Smiling face |
| 11 | `happy-open` | Happy face (mouth open) |
| 12 | `happy-closed` | Happy face (mouth closed) |
| 13 | `heart` | Heart shape |
| 14 | `big-surprise` | Big surprise expression |
| 15 | `small-surprise` | Small surprise expression |
| 16 | `tongue-out` | Tongue sticking out |
| 17 | `vamp1` | Vampire teeth variant 1 |
| 18 | `vamp2` | Vampire teeth variant 2 |
| 19 | `line` | Straight line |
| 20 | `confused` | Confused expression |
| 21 | `diagonal` | Diagonal line |
| 22 | `sad` | Sad face |
| 23 | `sad-open` | Sad face (mouth open) |
| 24 | `sad-closed` | Sad face (mouth closed) |
| 25 | `ok` | OK symbol |
| 26 | `x` | X mark |
| 27 | `interrogation` | Question mark |
| 28 | `thunder` | Lightning bolt |
| 29 | `culito` | Butt shape |
| 30 | `angry` | Angry expression |

### List available mouths

```bash
zowi_cli mouth --list
```

Output:

```
Available mouths (0-30):
  0: zero         1: one           2: two          3: three
  4: four         5: five          6: six          7: seven
  8: eight        9: nine         10: smile       11: happy-open
 12: happy-closed 13: heart       14: big-surprise 15: small-surprise
 16: tongue-out  17: vamp1        18: vamp2       19: line
 20: confused     21: diagonal    22: sad         23: sad-open
 24: sad-closed   25: ok          26: x           27: interrogation
 28: thunder      29: culito      30: angry
```

### Basic usage

```bash
zowi_cli mouth smile
zowi_cli mouth heart
zowi_cli mouth 10          # same as smile (by ID)
```

### Options

| Option | Description |
|--------|-------------|
| `mouth` | Mouth name or ID (0-30) (positional argument) |
| `-a, --address` | Robot Bluetooth address (overrides paired device) |
| `-t, --timeout` | Timeout waiting for connection (seconds, default `3`) |
| `--backend` | `auto` (registered transport), `bluetooth`, or `usb` |
| `--tty` | Serial TTY for USB (e.g. `/dev/ttyUSB0`) |
| `--baud` | Serial baud rate (default `115200`) |
| `-l, --list` | List available mouths and exit |

### Notes

- The mouth pattern stays displayed until changed by another `mouth` command or
  a gesture (gestures temporarily override the mouth).
- You can specify the mouth by name (case-insensitive, hyphens accepted) or by
  numeric ID.
- The firmware protocol uses 0-based IDs (0-30); the CLI accepts both names and
  IDs.

## Sing

Play a melody/sound on the robot's buzzer. The robot has a small piezo buzzer
that can play simple tones and melodies.

### Available melodies

| ID | Name | Description |
|----|------|-------------|
| 1 | `connection` | Connection sound |
| 2 | `disconnection` | Disconnection sound |
| 3 | `surprise` | Surprise sound |
| 4 | `oh-oh` | "Oh oh" sound |
| 5 | `oh-oh-2` | "Oh oh" variant 2 |
| 6 | `cuddly` | Cuddly/affectionate sound |
| 7 | `sleeping` | Sleeping/snoring sound |
| 8 | `happy` | Happy sound |
| 9 | `super-happy` | Very happy sound |
| 10 | `happy-short` | Short happy sound |
| 11 | `sad` | Sad sound |
| 12 | `confused` | Confused sound |
| 13 | `fart1` | Fart sound variant 1 |
| 14 | `fart2` | Fart sound variant 2 |
| 15 | `fart3` | Fart sound variant 3 |
| 16 | `mode1` | Mode 1 sound |
| 17 | `mode2` | Mode 2 sound |
| 18 | `mode3` | Mode 3 sound |
| 19 | `button-pushed` | Button pressed sound |

### List available melodies

```bash
zowi_cli sing --list
```

Output:

```
Available melodies (1-19):
  1: connection    2: disconnection  3: surprise      4: oh-oh
  5: oh-oh-2       6: cuddly         7: sleeping      8: happy
  9: super-happy  10: happy-short   11: sad         12: confused
 13: fart1        14: fart2         15: fart3       16: mode1
 17: mode2        18: mode3         19: button-pushed
```

### Basic usage

```bash
zowi_cli sing happy
zowi_cli sing connection
zowi_cli sing 8            # same as happy (by ID)
```

### Options

| Option | Description |
|--------|-------------|
| `melody` | Melody name or ID (1-19) (positional argument) |
| `-a, --address` | Robot Bluetooth address (overrides paired device) |
| `-t, --timeout` | Timeout waiting for connection (seconds, default `3`) |
| `--backend` | `auto` (registered transport), `bluetooth`, or `usb` |
| `--tty` | Serial TTY for USB (e.g. `/dev/ttyUSB0`) |
| `--baud` | Serial baud rate (default `115200`) |
| `-l, --list` | List available melodies and exit |

### Notes

- Melodies are blocking: the robot plays the full melody before accepting the
  next command.
- You can specify the melody by name (case-insensitive, hyphens accepted) or by
  numeric ID.
- The firmware protocol uses 1-based IDs (1-19); the CLI accepts both names and
  IDs.
- Some melodies (like `fart1`, `fart2`, `fart3`) are meant to be combined with
  gestures for comedic effect.

## Calibrate

Calibrate Zowi's four servo trims (protocol commands `C`/`G`). Each servo —
left leg `YL`, right leg `YR`, left foot `RL`, right foot `RR` — gets a signed
offset (in degrees) added to every commanded angle, compensating for mechanical
misalignment. The robot stores the trims in EEPROM, so they survive power cycles
and are re-applied automatically on every boot.

`calibrate` connects to the paired robot (or the `--address` target) and works in
one of two modes:

- **Interactive wizard** (default, requires a terminal) — mirrors the Android
  app's flow: WARNING → LEGS → FEET → CHECK, sending live `G` commands so the
  robot moves as you adjust.
- **Direct mode** — pass all four trims (`--yl --yr --rl --rr`) to persist them
  with a single `C` command. Useful for scripting/automation.

Trims are clamped to **±60°**, matching the servo's physical working range around
the 90° neutral position (the stock Android app only allowed ±30°). In direct
mode an out-of-range value is clamped with a warning; the interactive wizard
simply stops at the limit.

A **VICTORY** animation is played after saving, unless skipped with
`--no-victory` (`-N`).

### Options

| Option | Description |
|--------|-------------|
| `-a, --address` | Robot Bluetooth address to override the paired device (or a USB TTY path) |
| `-t, --timeout` | Timeout waiting for connection (seconds, default `3`) |
| `--backend` | `auto` (registered transport), `bluetooth`, or `usb` |
| `--tty` | Serial TTY for USB (e.g. `/dev/ttyUSB0`) |
| `--baud` | Serial baud rate (control firmware uses `115200`) |
| `-N, --no-victory` | Skip the victory animation when calibration is confirmed |
| `--yl` | Left leg trim (direct mode) |
| `--yr` | Right leg trim (direct mode) |
| `--rl` | Left foot trim (direct mode) |
| `--rr` | Right foot trim (direct mode) |

### Interactive wizard

```bash
zowi_cli calibrate
```

Screens, in order:

1. **WARNING** — explains that the servos will be moved and that trims are saved
   permanently. `y` continues, `x` cancels.
2. **LEGS** — adjust `YL` and `YR`; `n` advances to the feet.
3. **FEET** — adjust `RL` and `RR`; `n` advances to the check screen.
4. **CHECK** — `t` test movement, `r` restart, `c` confirm & save.

Key layout while adjusting (LEGS/FEET):

| Key | Action |
|-----|--------|
| `←` / `a` | select the left servo of the pair |
| `→` / `d` | select the right servo of the pair |
| `↑` | increase the selected trim by 10° |
| `↓` | decrease the selected trim by 10° |
| `+` | increase the selected trim by 1° |
| `-` | decrease the selected trim by 1° |
| `n` | next step |
| `x` / `q` / `ESC` / `Ctrl-C` | cancel (trims unchanged) |

Consecutive changes are sent no faster than every 200 ms (the firmware moves
each servo for ~200 ms). Calibration requires a terminal; without one, pass the
four trims for direct mode.

### Direct mode

```bash
zowi_cli calibrate --yl 40 --yr 0 --rl -8 --rr 3 -N
```

Sets and persists the four trims, waits for the robot's EEPROM final ack
(`&&F`), and reports the result:

```
Setting trims: YL=40 YR=0 RL=-8 RR=3
Trims saved to EEPROM.
```

Out-of-range values are clamped:

```
Warning: YR trim 75 clamped to 60 (range -60..60).
```

### Requires a connected robot

`calibrate` needs a reachable robot (paired device or explicit `--address`).
With an empty session it fails cleanly:

```
No paired device found. Run 'connect' first or pass --address.
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