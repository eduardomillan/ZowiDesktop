#!/usr/bin/env bash
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
RESTORE_TIMEOUT="${RESTORE_TIMEOUT:-15}"
FIRMWARE_PATH="${ZOWI_FACTORY_FIRMWARE:-src/firmware/ZOWI_BASE_v2.hex}"

echo "=== Checking current pairing status ==="
"$CLI" status

echo ""
echo "=== Restoring factory firmware ==="
"$CLI" restore -f "$FIRMWARE_PATH" -t "$RESTORE_TIMEOUT" --force-low-battery

echo ""
echo "=== Verifying status after restore ==="
"$CLI" status

echo ""
echo "PASS: Factory firmware restore command completed."
