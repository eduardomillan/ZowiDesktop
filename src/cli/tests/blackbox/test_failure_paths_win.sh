#!/usr/bin/env bash
# Windows black-box test of the clean-failure / no-hardware paths that are
# stable across backends: an empty session must make `rename` fail cleanly (with
# a clear message and non-zero exit) and `status` report that no robot is
# connected.
#
# Unlike the Linux counterpart (test_failure_paths.sh), this does NOT exercise
# the `control` over Bluetooth/USB reachability paths: on Windows the backend is
# bt_native (WinRT) / WinSerial COM ports, whose error strings differ from
# Linux/BlueZ and are not reliably assertable in CI.
#
# State isolation is handled by the orchestrator setting a fresh APPDATA dir, so
# `rename`/`status` see an empty session.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/Release/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

# The CLI path may be a Windows-style absolute path when ZOWI_CLI is not the
# default; use a plain existence check so it works in MSYS/Git Bash.
if [ ! -f "$CLI" ]; then
    fail "zowi_cli not found at $CLI (build it first, e.g. via the Windows CI)"
fi

LOG="$(mktemp)"

echo "=== Step 1: rename with no paired device fails cleanly ==="
if "$CLI" rename "TestName" >"$LOG" 2>&1; then
    fail "rename with an empty session unexpectedly succeeded"
fi
grep -qi "No paired device" "$LOG" || fail "rename did not report the missing device"
echo "ok: rename fails cleanly with no paired device"

echo "=== Step 2: status with no paired device reports no robot ==="
STATUS=$("$CLI" status -t 1 2>&1) || fail "status failed to run"
grep -qi "No Zowi connected" <<<"$STATUS" || fail "status did not report 'No Zowi connected'"
echo "ok: status reports no robot when the session is empty"

rm -f "$LOG"

echo "All Windows clean-failure-path tests passed."
