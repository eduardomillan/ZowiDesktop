#!/usr/bin/env bash
# USB analogue of bt/test_install_adivinawi.sh: installs the Adivinawi game
# firmware over a USB serial link (--backend usb) instead of Bluetooth.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
ADIVINAWI_TIMEOUT="${ADIVINAWI_TIMEOUT:-15}"
FIRMWARE_PATH="${ZOWI_ADIVINAWI_FIRMWARE:-src/firmware/ZOWI_Adivinawi_v2.hex}"
USB_TTY="${ZOWI_USB_TTY:-}"
USB_BAUD="${ZOWI_USB_BAUD:-115200}"

TTY_ARGS=()
if [ -n "$USB_TTY" ]; then
    TTY_ARGS=(--tty "$USB_TTY")
fi

echo "=== Listing USB serial ports ==="
"$CLI" ports

echo ""
echo "=== Installing Adivinawi game firmware over USB ==="
"$CLI" adivinawi -f "$FIRMWARE_PATH" -t "$ADIVINAWI_TIMEOUT" \
    --backend usb --baud "$USB_BAUD" "${TTY_ARGS[@]}" --force-low-battery

echo ""
echo "PASS: Adivinawi game firmware install over USB command completed."
