# CLI Test Suites

Black-box tests for the `zowi_cli` command-line tool. They drive the compiled
binary as an external process and assert on its stdout, stderr and exit codes.

There are two tiers:

| Tier | Location | Hardware required | Runs in CI |
|------|----------|-------------------|------------|
| **Black-box, no hardware** | `blackbox/` | No | Yes (`cli_blackbox` via CTest) |
| **Black-box, no hardware (Windows)** | `blackbox/` (backend-agnostic cases) | No | Yes (`cli_blackbox_win` via CTest) |
| **Bluetooth hardware** | `bt/` | A real Zowi in range (BlueZ) | No |
| **USB hardware** | `usb/` | A real Zowi over USB serial | No |

## Quick start

Build the CLI and run the full automated suite (white-box + no-hardware
black-box) in one command:

```bash
./run-tests.sh
```

Options: `--verbose` (also print the black-box CLI detail), `--no-build`
(reuse an existing build), `-5`/`-6` (Qt version on Linux). The equivalent
lower-level commands are:

```bash
./build.sh --cli
ctest --test-dir build --output-on-failure
```

The `cli_blackbox` CTest entry is a thin wrapper around
`scripts/test/run-cli-blackbox.sh`, the canonical no-hardware orchestrator. Run
it directly for more verbose output:

```bash
scripts/test/run-cli-blackbox.sh
```

### Environment (no-hardware suite)

- `ZOWI_CLI` — path to the `zowi_cli` binary (default `build/src/cli/zowi_cli`).
- `NO_BUILD` — set to `1` to skip auto-building the CLI.

The orchestrator points `XDG_CONFIG_HOME` at a throwaway temp dir so the
`session`/`status`/`rename` tests never touch your real ZowiDesktop session
data, and it runs from the repository root so `translate`/`config` resolve
`i18n/` and `src/config.json`.

## Windows coverage

A parallel orchestrator, `scripts/test/run-cli-blackbox-win.sh`, runs in CI on
`windows-latest` (via the `tests-windows.yml` workflow and the `cli_blackbox_win`
CTest entry). It is the Windows analogue of `run-cli-blackbox.sh`, with these
differences:

- Looks for the binary at `build/src/cli/Release/zowi_cli` (the MSVC Release
  path; resolves the `.exe` extension automatically).
- Isolates session state via the `APPDATA` env var (what `SessionStore` reads on
  Windows) instead of `XDG_CONFIG_HOME`.
- Runs only the **backend-agnostic** cases: `test_help`, `test_session`,
  `test_config`, `test_translate`, and `test_failure_paths_win` (empty-session
  `rename`/`status`). It does **not** run the USB/Bluetooth reachability cases,
  because on Windows the backend is `bt_native` (WinRT)/WinSerial and its output
  differs from Linux/BlueZ.

## Bluetooth hardware tests (`bt/`)

Talk to a real robot over Bluetooth (BlueZ SPP RFCOMM). Most require a robot in
range and paired; flashing tests require the firmware bundle (see
`scripts/sync_firmware_from_zowiLibs.sh`).

```bash
# Gracious no-robot paths only
./run_all.sh

# Full suite (needs a robot in range)
ZOWI_BT_FULL=1 ./run_all.sh
```

Environment:

- `ZOWI_CLI` — path to `zowi_cli`.
- `ZOWI_BT_FULL=1` — run the connect/rename, control and flashing tests that
  need a robot. Without it only the tests that degrade gracefully run.

Flashing over Bluetooth needs to bind an RFCOMM channel, so grant the CLI the
capability first (re-applied after every rebuild):

```bash
sudo setcap cap_net_admin+ep build/src/cli/zowi_cli
# or: scripts/grant_bluetooth_cap.sh
```

## USB hardware tests (`usb/`)

Drive a real robot connected over USB serial (e.g. `/dev/ttyUSB0`).

```bash
./run_all.sh                        # smoke tests, no robot needed
ZOWI_USB_FLASH=1   ./run_all.sh      # restore/alarm/adivinawi flashing
ZOWI_USB_CONNECT=1 ./run_all.sh      # connect/rename/disconnect over USB
```

Environment:

- `ZOWI_CLI` — path to `zowi_cli`.
- `ZOWI_USB_FLASH=1` — run the firmware flashing tests (needs a robot over USB).
- `ZOWI_USB_CONNECT=1` — run connect/rename/disconnect over USB (needs a robot).
- `ZOWI_USB_TTY` / `ZOWI_USB_BAUD` — optional port/baud overrides.

> **Note:** the CLI must be built with the USB serial backend. This requires the
> `ZOWI_HAVE_SERIAL` compile definition, which `src/cli/CMakeLists.txt` sets
> automatically (same as the GUI). If `zowi_cli ports` prints "not available on
> this platform", rebuild with `./build.sh --cli`.

## Adding a test

- **No hardware**: drop a `test_<topic>.sh` in `blackbox/` and add a `run`
  line to `scripts/test/run-cli-blackbox.sh`. Follow the existing style:
  `set -euo pipefail`, a `fail()` helper, and assertions on `--help`, stdout or
  exit codes.
- **Hardware**: add it to the relevant `bt/` or `usb/` directory and wire it
  into that directory's `run_all.sh`. It will not run in CI.
