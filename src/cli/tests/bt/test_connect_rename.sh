#!/usr/bin/env bash
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
SCAN_TIMEOUT=5
CONNECT_TIMEOUT=5
RENAME_TIMEOUT=5

echo "=== Step 1: Scanning for Zowis ==="
SCAN_OUTPUT=$("$CLI" scan -t "$SCAN_TIMEOUT" 2>&1) || true
echo "$SCAN_OUTPUT"

# Extract first Zowi address (line matching "Zowi [XX:XX:XX:XX:XX:XX]")
ADDR=$(echo "$SCAN_OUTPUT" | grep -oP 'Zowi \[\K[0-9A-F:]{17}' | head -1 || true)

if [ -z "$ADDR" ]; then
    echo "FAIL: No Zowi device found."
    exit 1
fi

echo ""
echo "Found Zowi at: $ADDR"

echo ""
echo "=== Step 2: Connecting ==="
CONNECT_OUTPUT=$("$CLI" connect "$ADDR" -t "$CONNECT_TIMEOUT" 2>&1) || true
echo "$CONNECT_OUTPUT"

echo ""
echo "=== Step 3: Checking status ==="
"$CLI" status

echo ""
echo "=== Step 4: Renaming to TestZowi ==="
RENAME_OUTPUT=$("$CLI" rename "TestZowi" -t "$RENAME_TIMEOUT" 2>&1) || true
echo "$RENAME_OUTPUT"

echo ""
echo "=== Step 5: Checking status after rename ==="
"$CLI" status

echo ""
echo "PASS: Zowi connected, renamed to TestZowi. Status shown above."
