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
zowi_cli scan --all
zowi_cli scan -a        # short form
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

## Examples

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
```
