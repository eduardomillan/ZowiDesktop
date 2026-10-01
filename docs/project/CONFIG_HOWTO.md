# Configuration — How To

> How Zowi Desktop is configured: the compiled `config.json`, the optional
> override layers (system file, environment variables, user file) and every key
> with what it does and its possible values.

## Table of Contents

- [How it works](#how-it-works)
  - [Layers and priority](#layers-and-priority)
  - [Where the files are](#where-the-files-are)
  - [Switches that enable the lower layers](#switches-that-enable-the-lower-layers)
  - [Environment variables](#environment-variables)
  - [Value formats](#value-formats)
  - [The Debian package](#the-debian-package)
- [Recipes](#recipes)
- [Keys](#keys)
  - [Configuration layers](#configuration-layers)
  - [Ranking](#ranking)
  - [Logging and development](#logging-and-development)
  - [Window and messages](#window-and-messages)
  - [Help dialogs and games](#help-dialogs-and-games)
  - [Timeline](#timeline)
  - [Connection and robot](#connection-and-robot)
  - [Firmware restore](#firmware-restore)
  - [Links and images](#links-and-images)
  - [Colors](#colors)

---

## How it works

`src/config.json` is **compiled into the app** (the GUI reads the Qt resource
`:/src/config.json`; the CLI reads the file `src/config.json` from the workspace)
and holds the default value of every key. So that one installation can behave
differently without recompiling, the app applies up to three **override layers**
on top of it, for both the GUI and `zowi_cli`.

### Layers and priority

From highest to lowest priority:

| # | Layer | Used when |
|---|-------|-----------|
| 1 | **System file** | always, if it exists |
| 2 | **Environment variables** `ZOWI_<KEY>` | `allow_environment_config` is `true` |
| 3 | **User file** | `allow_user_config` is `true` |
| 4 | **Compiled `config.json`** | always (the defaults) |

Each key is resolved on its own: **if a layer does not define a key, the value of
the next lower layer applies.** The system file wins over everything: a key set
there cannot be changed by the environment or the user. Only the administrator
edits it.

### Where the files are

| Layer | Linux | Windows |
|-------|-------|---------|
| System | `/etc/ZowiDesktop/config.json` | `%PROGRAMDATA%\ZowiDesktop\config.json` |
| User | `~/.config/ZowiDesktop/config.json` (or `$XDG_CONFIG_HOME/ZowiDesktop/config.json`) | `%APPDATA%\ZowiDesktop\config.json` |

The user file lives in the same folder as the session file `ZowiApp.json`.
A file that does not exist, is not valid JSON or is not a JSON object is ignored
(with a warning on stderr). Files only need the keys they change.

### Switches that enable the lower layers

`allow_environment_config` and `allow_user_config` are **`false` in the compiled
config**: the environment and the user file are ignored until the **system file**
turns them on. These two keys are read **only** from the system file (or the
compiled defaults); the user file and the environment cannot set them.

An installation without a system file therefore uses the compiled values only.
To lock a setting there are two ways: put that key in the system file (it can't be
changed from below), or leave both switches at `false`.

### Environment variables

`ZOWI_` followed by the key in upper case: `ranking_enabled` →
`ZOWI_RANKING_ENABLED`. An empty variable is ignored. Handy for a one-off run:

```bash
ZOWI_RANKING_ENABLED=false ./ZowiDesktop      # needs allow_environment_config=true in the system file
```

`DEV_MODE` (no prefix) is a separate, older switch that forces development mode
on or off for a run (`1`, `true`, `on`); it is not affected by the layers.

### Value formats

Values are text, as in `config.json`. Yes/no keys accept `true` / `false` or
`1` / `0`, in any case. In the system and user files you may also write a JSON
boolean (`true`) or number (`2000`); they are stored as text.

### The Debian package

The `.deb` installs `/etc/ZowiDesktop/config.json` as a **configuration file that
upgrades do not overwrite**, containing:

```json
{
    "dev_mode": "false",
    "log_level": "warn"
}
```

The administrator adds there the keys to change, for example `ranking_enabled`
or the two `allow_*` switches. The AppImage and the Windows builds do not
install a system file (create it by hand if needed).

## Recipes

Turn the ranking off on a school installation (nobody can turn it back on):

```json
{
    "ranking_enabled": "false"
}
```

Let users and the environment adjust settings, except the ranking:

```json
{
    "allow_user_config": "true",
    "allow_environment_config": "true",
    "ranking_enabled": "false"
}
```

Let a user choose their own start window size (system file allows it, user file
sets it):

```json
{ "allow_user_config": "true" }
```

```json
{ "window_size_ratio": "0.6" }
```

## Keys

Default values are the ones in `src/config.json`. "Read by" says who uses the key.

### Configuration layers

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `allow_environment_config` | `false` | `true` / `false`. Lets `ZOWI_<KEY>` variables override keys. Only honoured in the system file or the compiled config. |
| `allow_user_config` | `false` | `true` / `false`. Lets the user file override keys. Only honoured in the system file or the compiled config. |

### Ranking

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `ranking_enabled` | `true` | `true` / `false`. When `false` the ranking is hidden everywhere (buttons, dialog, help paragraph, messages) and nothing is recorded; data already saved is kept and returns when it is turned on again. `zowi_cli ranking` still works. See `RANKING_HOWTO.md`. |
| `ranking_online_allowed` | `true` | `true` / `false`. When `false` everything about the **world ranking** is hidden and no request is ever made (set it in the system file to forbid it in a whole installation). Has no effect until the two URLs below are set. |
| `ranking_online_read_url` | *(empty)* | `https://` address of the public world-ranking JSON (or `http://localhost…` for development). While empty, the "World" tab does not exist. |
| `ranking_online_submit_url` | *(empty)* | `https://` address of the server that registers scores (used when the user chooses to share). |

### Logging and development

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `log_level` | `info` | Minimum severity shown in the terminal: `debug`, `info`, `warn` (or `warning`), `error`. The daily log file always keeps the full history. In `zowi_cli`, only `debug` changes anything (it shows debug messages). The Debian package sets `warn`. |
| `dev_mode` | `false` | `true` / `false`. Shows the development overlay (reset buttons and similar). The `DEV_MODE` environment variable overrides it. The Debian package sets `false`. |

### Window and messages

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `window_size_ratio` | `0.75` | Number. Window size at start as a fraction of the screen (width and height). Invalid values fall back to `0.75`. The window never gets smaller than 480×360. |
| `message_duration` | `2000` | Milliseconds a transient message bar stays visible (Settings, Splash, Timeline). Invalid values fall back to `2000`. |
| `button_quit_visible` | `false` | `true` shows a **Quit** button on the start screen. |
| `button_scan_visible` | `false` | `true` shows the **Scan** button on the scan screen. |

### Help dialogs and games

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `memory_help` | `always` | Memory game "How to play": `always` opens it every time you enter; any other value opens it only the first time. |
| `mouths_help` | `always` | Same for the Draw the mouths game. |
| `timeline_help` | `always` | Same for the Timeline editor. |
| `mouths_target_onscreen` | `always` | Draw the mouths: where the target mouth is shown on screen. `always`, `never`, or `auto` (only while the robot is not connected). |

### Timeline

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `timeline_selectors_as_dialogs` | `true` | `true` opens the movement, gesture and mouth pickers as dialogs; `false` opens them as full screens. |
| `timeline_selector_dialog_size_ratio` | `0.8` | Number between 0 and 1: size of those picker dialogs as a fraction of the window. |

### Connection and robot

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `transport` | `auto` | Not read at the moment: the transport preference (`auto`, `bt`, `usb`) is stored in the session file. |
| `usb_baud` | `115200` | Serial speed used to talk to the robot over USB. Integer. |
| `usb_bootloader_baud` | `115200` | Serial speed used when flashing over USB. Integer. |
| `transport_timeout` | `1500` | Milliseconds to wait for a transport (Bluetooth / USB) to respond while detecting it. |
| `connect_timeout` | `10000` | Milliseconds to wait for a connection before giving up. |
| `rename_lock_ms` | `3000` | Milliseconds the rename screen stays locked while the new name is sent. Invalid values fall back to `1500`. |
| `zowi_mac_prefix` | `B4:9D:0B:3` | Start of the Bluetooth address of Zowi robots; the scan screen and `zowi_cli scan` use it to show only Zowis. |
| `zowi_default_name` | `Zowi` | Name proposed for a robot and used as the factory name when forgetting one. |

### Firmware restore

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `factory_firmware_path` | `qrc:/firmware/ZOWI_BASE_v2.hex` | Firmware file used by Settings → restore factory firmware. |
| `restore_low_battery_threshold` | `50` | Battery percentage below which restoring asks for confirmation. |
| `restore_simulate_low_battery` | `false` | `true` forces the low-battery warning, for testing the dialog. |

### Links and images

| Key | Default | Meaning and values |
|-----|---------|--------------------|
| `know_more` | `https://eduardomillan.github.io/ZowiDesktop` | Web page opened by the "know more" link (the language is appended to the address). |
| `hospital_url` | `https://eduardomillan.github.io/ZowiDesktop` | Web page opened by the "hospital" entry in Settings. |
| `splash_image` | `qrc:/images/android/hello_image.png` | Logo on the start screen. |
| `start_image` | `qrc:/images/android/pressed_animation_sleppy_button.png` | Image on the welcome screen. |
| `welcome_image` | `qrc:/images/android/welcome_image.png` | Image on the first wizard page. |
| `zowi_found_image` | `qrc:/images/android/zowi_found_image.png` | Image when a Zowi is found (wizard, rename). |
| `zowi_not_found_image` | `qrc:/images/android/zowi_not_found_image.png` | Not read at the moment. |

### Colors

All colors are CSS-style text (`#rrggbb`). Where the key is missing the app uses
the same value as a built-in fallback.

| Key | Default | Used for |
|-----|---------|----------|
| `color_primary` | `#2d5a2d` | Main text and outlines. |
| `color_accent` | `#21a69b` | Accent buttons, highlights. |
| `color_accent_pressed` | `#17736c` | Accent buttons while pressed. |
| `color_error` | `#c0392b` | Error messages. |
| `color_danger` | `#e74c3c` | Destructive actions (disconnect, forget). |
| `color_warning` | `#e67e22` | Warnings and notices. |
| `color_warning_text` | `#f1c40f` | Text on the message bar. |
| `color_bg_app` | `#f4f9f4` | App background. |
| `color_bg_hover` | `#e0f0e0` | Hover / highlighted rows. |
| `color_bg_connecting` | `#eaf2fb` | Status bar while connecting. |
| `color_bg_connected` | `#e8f5e8` | Connected state backgrounds. |
| `color_bg_low_battery` | `#fdecea` | Status bar with low battery. |
| `color_bg_disconnected` | `#fff3e0` | Status bar when disconnected. |
| `color_bg_disabled` | `#e6e6e6` | Disabled controls. |
| `color_fg_disabled` | `#9e9e9e` | Text on disabled controls. |
| `color_border_disabled` | `#c8c8c8` | Borders of disabled controls. |
| `color_fg_connecting` | `#2980b9` | Text while connecting. |
| `color_firmware` | `#8a6d1f` | Firmware / update indicators. |
