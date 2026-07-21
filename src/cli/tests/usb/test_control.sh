#!/usr/bin/env bash
# Smoke test for the `control` minigame subcommand over USB.
# Verifies argument parsing and the no-connection path without requiring a
# real robot (uses a non-existent port and a short timeout so it fails fast).
set -euo pipefail

CLI="${CLI:-./build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"

# 1. --help lists the subcommand and its options.
"$CLI" control --help 2>&1 | grep -q -- "--speed" || fail "control --help missing --speed option"
"$CLI" control --help 2>&1 | grep -q -- "--address" || fail "control --help missing --address option"
"$CLI" control --help 2>&1 | grep -q -- "--backend" || fail "control --help missing --backend option"
"$CLI" control --help 2>&1 | grep -q -- "--tty" || fail "control --help missing --tty option"
"$CLI" control --help 2>&1 | grep -q -- "--baud" || fail "control --help missing --baud option"
echo "ok: control --help shows expected options"

# 2. A non-existent USB port must fail to connect and exit non-zero (bounded by -t).
if "$CLI" control --backend usb --tty /dev/ttyUSB99 -t 1 >/tmp/control_usb_out.log 2>&1; then
    fail "control connected to a non-existent USB port (unexpected)"
fi
grep -qi "Could not connect\|No such file\|not found" /tmp/control_usb_out.log || fail "expected a connection failure message"
echo "ok: control fails cleanly when the USB port is unavailable"

echo "All USB control smoke tests passed."
