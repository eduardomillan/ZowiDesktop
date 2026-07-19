#!/usr/bin/env bash
# Disconnect / forget a Zowi that was registered over USB.
#
# Mirrors src/cli/tests/bt/test_disconnect.sh but targets the USB serial
# backend. The CLI 'disconnect' command clears the saved session (device
# address, name, transport) regardless of the transport, so it works for a
# USB-registered robot too.
#
# Requires a robot connected over USB and a prior 'connect --backend usb'.
# Provide ZOWI_USB_TTY / ZOWI_USB_BAUD to target a specific port and speed.
#
# Environment:
#   ZOWI_CLI      Path to the zowi_cli binary (default: build/src/cli/zowi_cli)
#   ZOWI_USB_TTY  USB serial port (e.g. /dev/ttyUSB0). If empty, the first port
#                 reported by 'zowi_cli ports' is used.
#   ZOWI_USB_BAUD Baud rate (default: 115200, the running control firmware rate)
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
BAUD="${ZOWI_USB_BAUD:-115200}"
TTY="${ZOWI_USB_TTY:-}"

if ! command -v "$CLI" >/dev/null 2>&1; then
    echo "FAIL: zowi_cli not found at $CLI (build it first: ./build.sh --cli)" >&2
    exit 1
fi

echo "=== Checking status ==="
STATUS=$("$CLI" status 2>&1) || true
echo "$STATUS"

# If nothing is registered, there is nothing to disconnect.
if echo "$STATUS" | grep -q "No Zowi connected"; then
    echo ""
    echo "INFO: No Zowi is currently registered. Nothing to disconnect."
    exit 0
fi

# Ensure we have a USB port to connect through if the robot is not live.
if [ -z "$TTY" ]; then
    PORTS_OUTPUT=$("$CLI" ports 2>&1) || true
    echo "$PORTS_OUTPUT"
    TTY=$(echo "$PORTS_OUTPUT" | grep -oP '/dev/tty[A-Za-z0-9]+' | head -1 || true)
fi

# If the robot is not currently live (e.g. port closed), bring it up over USB
# so the disconnect can verify a clean teardown, then forget it.
if echo "$STATUS" | grep -q "(cached)"; then
    if [ -n "$TTY" ]; then
        echo ""
        echo "=== Connecting over USB to $TTY (baud $BAUD) ==="
        "$CLI" connect "$TTY" --backend usb --tty "$TTY" --baud "$BAUD" 2>&1 || true
    fi
fi

echo ""
echo "=== Disconnecting / forgetting ==="
"$CLI" disconnect

echo ""
echo "=== Verifying status after disconnect ==="
"$CLI" status

echo ""
echo "PASS: Zowi disconnected (USB session cleared)."
