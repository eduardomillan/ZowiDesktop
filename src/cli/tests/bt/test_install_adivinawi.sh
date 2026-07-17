#!/usr/bin/env bash
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
ADIVINAWI_TIMEOUT="${ADIVINAWI_TIMEOUT:-15}"
FIRMWARE_PATH="${ZOWI_ADIVINAWI_FIRMWARE:-src/firmware/ZOWI_Adivinawi_v2.hex}"

echo "=== Checking current pairing status ==="
"$CLI" status

echo ""
echo "=== Installing Adivinawi game firmware ==="
"$CLI" adivinawi -f "$FIRMWARE_PATH" -t "$ADIVINAWI_TIMEOUT" --force-low-battery

echo ""
echo "=== Verifying status after install ==="
"$CLI" status

echo ""
echo "PASS: Adivinawi game firmware install command completed."
