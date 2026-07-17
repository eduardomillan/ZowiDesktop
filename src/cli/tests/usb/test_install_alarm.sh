#!/usr/bin/env bash
# USB analogue of bt/test_install_alarm.sh: installs the Robot Alarm firmware
# over a USB serial link (--backend usb) instead of Bluetooth.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
ALARM_TIMEOUT="${ALARM_TIMEOUT:-15}"
FIRMWARE_PATH="${ZOWI_ALARM_FIRMWARE:-src/firmware/ZOWI_Alarm_v2.hex}"
USB_TTY="${ZOWI_USB_TTY:-}"
USB_BAUD="${ZOWI_USB_BAUD:-115200}"

TTY_ARGS=()
if [ -n "$USB_TTY" ]; then
    TTY_ARGS=(--tty "$USB_TTY")
fi

echo "=== Listing USB serial ports ==="
"$CLI" ports

echo ""
echo "=== Installing Robot Alarm firmware over USB ==="
"$CLI" alarm -f "$FIRMWARE_PATH" -t "$ALARM_TIMEOUT" \
    --backend usb --baud "$USB_BAUD" "${TTY_ARGS[@]}" --force-low-battery

echo ""
echo "PASS: Robot Alarm firmware install over USB command completed."
