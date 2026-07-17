#!/usr/bin/env bash
# USB analogue of the Bluetooth `scan` test: instead of discovering robots over
# Bluetooth, the USB workflow enumerates serial ports (/dev/ttyUSB*, /dev/ttyACM*)
# with the `ports` subcommand.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"

echo "=== Step 1: Listing USB serial ports ==="
PORTS_OUTPUT=$("$CLI" ports 2>&1) || true
echo "$PORTS_OUTPUT"

# The command must always succeed and print a deterministic header, whether or
# not a robot is plugged in.
if echo "$PORTS_OUTPUT" | grep -q "No USB serial ports found"; then
    echo ""
    echo "INFO: No USB serial ports present."
elif echo "$PORTS_OUTPUT" | grep -q "Available USB serial ports:"; then
    PORT=$(echo "$PORTS_OUTPUT" | grep -oP '/dev/tty(USB|ACM)[0-9]+' | head -1 || true)
    echo ""
    echo "Found USB serial port: $PORT"
else
    fail "ports produced unexpected output"
fi

echo ""
echo "PASS: USB serial port enumeration command completed."
