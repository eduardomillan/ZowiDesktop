#!/usr/bin/env bash
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
ALARM_TIMEOUT="${ALARM_TIMEOUT:-15}"
FIRMWARE_PATH="${ZOWI_ALARM_FIRMWARE:-src/firmware/ZOWI_Alarm_v2.hex}"

echo "=== Checking current pairing status ==="
"$CLI" status

echo ""
echo "=== Installing Robot Alarm firmware ==="
"$CLI" alarm -f "$FIRMWARE_PATH" -t "$ALARM_TIMEOUT" --force-low-battery

echo ""
echo "=== Verifying status after install ==="
"$CLI" status

echo ""
echo "PASS: Robot Alarm firmware install command completed."
