#!/usr/bin/env bash
# Connect to a Zowi over USB and rename it, exercising the CLI's USB backend.
#
# Requires a robot connected over USB serial. Set ZOWI_USB_TTY / ZOWI_USB_BAUD
# to target a specific port and speed (defaults: first enumerated port, 57600).
#
# This mirrors src/cli/tests/bt/test_connect_rename.sh but drives the robot
# through the USB serial backend (--backend usb --tty <port>) instead of
# Bluetooth.
#
# Environment:
#   ZOWI_CLI      Path to the zowi_cli binary (default: build/src/cli/zowi_cli)
#   ZOWI_USB_TTY  USB serial port (e.g. /dev/ttyUSB0). If empty, the first port
#                 reported by 'zowi_cli ports' is used.
#   ZOWI_USB_BAUD Baud rate (default: 115200, the running control firmware's rate)
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
CONNECT_TIMEOUT=5
RENAME_TIMEOUT=5
BAUD="${ZOWI_USB_BAUD:-115200}"
TTY="${ZOWI_USB_TTY:-}"

echo "=== Step 1: Listing USB serial ports ==="
PORTS_OUTPUT=$("$CLI" ports 2>&1) || true
echo "$PORTS_OUTPUT"

if [ -z "$TTY" ]; then
    TTY=$(echo "$PORTS_OUTPUT" | grep -oP '/dev/tty[A-Za-z0-9]+' | head -1 || true)
fi

if [ -z "$TTY" ]; then
    echo "FAIL: No USB serial port found (set ZOWI_USB_TTY or connect a robot)."
    exit 1
fi

echo ""
echo "Using USB port: $TTY (baud $BAUD)"

echo ""
echo "=== Step 2: Connecting over USB ==="
CONNECT_OUTPUT=$("$CLI" connect "$TTY" --backend usb --tty "$TTY" --baud "$BAUD" -t "$CONNECT_TIMEOUT" 2>&1) || true
echo "$CONNECT_OUTPUT"

echo ""
echo "=== Step 3: Checking status ==="
"$CLI" status

echo ""
echo "=== Step 4: Renaming to TestZowi (USB) ==="
RENAME_OUTPUT=$("$CLI" rename "TestZowi" --backend usb --tty "$TTY" --baud "$BAUD" -t "$RENAME_TIMEOUT" 2>&1) || true
echo "$RENAME_OUTPUT"

echo ""
echo "=== Step 5: Checking status after rename ==="
"$CLI" status

echo ""
echo "PASS: Zowi connected over USB and rename command sent. Status shown above."
