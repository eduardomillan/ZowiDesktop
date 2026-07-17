#!/usr/bin/env bash
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"

echo "=== Checking status ==="
STATUS=$("$CLI" status 2>&1) || true
echo "$STATUS"

if echo "$STATUS" | grep -q "No Zowi connected"; then
    echo ""
    echo "INFO: No Zowi is currently paired. Nothing to disconnect."
    exit 0
fi

echo ""
echo "=== Disconnecting ==="
"$CLI" disconnect

echo ""
echo "=== Verifying status after disconnect ==="
"$CLI" status

echo ""
echo "PASS: Zowi disconnected."
