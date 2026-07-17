#!/usr/bin/env bash
# Smoke test for the `control` minigame subcommand.
# Verifies argument parsing and the no-connection path without requiring a
# real robot (uses an unroutable address and a short timeout so it fails fast).
set -euo pipefail

CLI="${CLI:-./build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"

# 1. --help lists the subcommand and its options.
"$CLI" control --help 2>&1 | grep -q -- "--speed" || fail "control --help missing --speed option"
"$CLI" control --help 2>&1 | grep -q -- "--address" || fail "control --help missing --address option"
echo "ok: control --help shows expected options"

# 2. An unreachable address must fail to connect and exit non-zero (bounded by -t).
if "$CLI" control --address 00:00:00:00:00:00 -t 1 >/tmp/control_out.log 2>&1; then
    fail "control connected to an unroutable address (unexpected)"
fi
grep -qi "Could not connect" /tmp/control_out.log || fail "expected a connection failure message"
echo "ok: control fails cleanly when the robot is unreachable"

echo "All control smoke tests passed."
