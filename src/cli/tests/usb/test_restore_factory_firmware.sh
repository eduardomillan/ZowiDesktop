#!/usr/bin/env bash
# USB analogue of bt/test_restore_factory_firmware.sh: restores the factory
# firmware over a USB serial link (--backend usb) instead of Bluetooth.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
RESTORE_TIMEOUT="${RESTORE_TIMEOUT:-15}"
FIRMWARE_PATH="${ZOWI_FACTORY_FIRMWARE:-src/firmware/ZOWI_BASE_v2.hex}"
USB_TTY="${ZOWI_USB_TTY:-}"
USB_BAUD="${ZOWI_USB_BAUD:-115200}"

TTY_ARGS=()
if [ -n "$USB_TTY" ]; then
    TTY_ARGS=(--tty "$USB_TTY")
fi

echo "=== Listing USB serial ports ==="
"$CLI" ports

echo ""
echo "=== Restoring factory firmware over USB ==="
"$CLI" restore -f "$FIRMWARE_PATH" -t "$RESTORE_TIMEOUT" \
    --backend usb --baud "$USB_BAUD" "${TTY_ARGS[@]}" --force-low-battery

echo ""
echo "PASS: Factory firmware restore over USB command completed."
