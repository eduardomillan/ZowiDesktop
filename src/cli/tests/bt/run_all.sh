#!/usr/bin/env bash
# Runs the Bluetooth CLI tests in order.
#
# These tests talk to a real robot over Bluetooth (BlueZ SPP). By default only
# the tests that degrade gracefully without a robot run (disconnect). Set
# ZOWI_BT_FULL=1 to also run the tests that require a robot in range
# (connect/rename and the firmware flashing tests).
#
# Environment:
#   ZOWI_CLI     Path to the zowi_cli binary (default: build/src/cli/zowi_cli)
#   ZOWI_BT_FULL Set to 1 to run tests that require a robot (connect, flashing)
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
FULL="${ZOWI_BT_FULL:-0}"

pass=0
fail=0
failed_tests=()

run() {
    local name="$1"; shift
    echo ""
    echo "########################################################"
    echo "# RUNNING: $name"
    echo "########################################################"
    if "$@"; then
        echo ">>> OK: $name"
        pass=$((pass + 1))
    else
        echo ">>> FAILED: $name"
        fail=$((fail + 1))
        failed_tests+=("$name")
    fi
}

if ! command -v "$CLI" >/dev/null 2>&1; then
    echo "FAIL: zowi_cli not found at $CLI (build it first: ./build.sh --cli)" >&2
    exit 1
fi
export ZOWI_CLI="$CLI"
export CLI

# 1. Runs whether or not a robot is present (reports no device gracefully).
run "bt/test_disconnect.sh" "$SCRIPT_DIR/test_disconnect.sh"

# 2. Tests that require a robot in range (opt-in).
if [ "$FULL" = "1" ]; then
    run "bt/test_connect_rename.sh"            "$SCRIPT_DIR/test_connect_rename.sh"
    run "bt/test_control.sh"                   "$SCRIPT_DIR/test_control.sh"
    run "bt/test_restore_factory_firmware.sh"  "$SCRIPT_DIR/test_restore_factory_firmware.sh"
    run "bt/test_install_alarm.sh"             "$SCRIPT_DIR/test_install_alarm.sh"
else
    echo ""
    echo "SKIP: connect/rename, control and flashing tests (set ZOWI_BT_FULL=1 with a robot in range)"
fi

echo ""
echo "========================================================"
echo "Bluetooth test summary: $pass passed, $fail failed"
if [ "$fail" -ne 0 ]; then
    printf '  - %s\n' "${failed_tests[@]}"
    exit 1
fi
echo "All Bluetooth tests passed."
