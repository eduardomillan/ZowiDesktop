#!/usr/bin/env bash
# USB analogue of bt/test_control.sh: a smoke test that verifies the USB-related
# argument parsing (--backend usb, --baud, --tty) and the clean-failure path
# when no robot is reachable, without requiring real hardware.
set -euo pipefail

CLI="${CLI:-./build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"

# 1. `ports` subcommand exists and is documented.
"$CLI" --help 2>&1 | grep -q -- "ports" || fail "top-level help missing 'ports' subcommand"
echo "ok: 'ports' subcommand is listed"

# 2. restore/alarm expose the USB options.
"$CLI" restore --help 2>&1 | grep -q -- "--backend" || fail "restore --help missing --backend option"
"$CLI" restore --help 2>&1 | grep -q -- "--baud"    || fail "restore --help missing --baud option"
"$CLI" restore --help 2>&1 | grep -qi "usb"         || fail "restore --backend help missing 'usb' mode"
"$CLI" alarm   --help 2>&1 | grep -q -- "--baud"    || fail "alarm --help missing --baud option"
echo "ok: restore/alarm expose --backend usb and --baud"

# 3. Requesting the USB backend with a non-existent TTY must fail cleanly (non-zero).
if "$CLI" restore --backend usb --tty /dev/tty_does_not_exist --baud 115200 \
        -t 1 --force-low-battery >/tmp/usb_out.log 2>&1; then
    fail "restore over USB succeeded with a non-existent TTY (unexpected)"
fi
echo "ok: restore over USB fails cleanly when the port is unavailable"

echo "All USB smoke tests passed."
