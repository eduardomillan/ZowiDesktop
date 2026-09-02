#!/usr/bin/env bash
# Black-box test of the clean-failure / no-hardware paths that do not require a
# real robot. These verify the CLI degrades gracefully (non-zero exit with a
# clear message) when asked to talk to a robot that is not reachable.
#
# All side-effect-free state isolation is handled by the orchestrator setting
# XDG_CONFIG_HOME to a fresh temp dir, so `rename`/`status` see an empty session.
#
# NOTE: `connect` intentionally succeeds even when no robot responds (it saves
# a session with "(not received)" and exits 0), so the reachability failure is
# exercised through `control`, which does exit non-zero when it cannot connect.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"

echo "=== Step 1: rename with no paired device fails cleanly ==="
if "$CLI" rename "TestName" >/tmp/rename_out.log 2>&1; then
    fail "rename with an empty session unexpectedly succeeded"
fi
grep -qi "No paired device" /tmp/rename_out.log || fail "rename did not report the missing device"
echo "ok: rename fails cleanly with no paired device"

echo "=== Step 2: status with no paired device reports no robot ==="
STATUS=$("$CLI" status -t 1 2>&1)
grep -qi "No Zowi connected" <<<"$STATUS" || fail "status did not report 'No Zowi connected'"
echo "ok: status reports no robot when the session is empty"

echo "=== Step 3: control over USB without a present port fails cleanly ==="
if "$CLI" control --backend usb --address /dev/ttyUSB99 -t 1 >/tmp/control_usb.log 2>&1; then
    fail "control over USB unexpectedly succeeded with no port"
fi
grep -qi "No USB serial ports found\|Plug in the robot\|not available" /tmp/control_usb.log \
    || fail "control did not report the missing USB port"
echo "ok: control fails cleanly over USB when no port is present"

echo "=== Step 4: control over Bluetooth with an unreachable address fails cleanly ==="
if "$CLI" control --backend bluetooth --address 00:00:00:00:00:00 -t 1 >/tmp/control_bt.log 2>&1; then
    fail "control over Bluetooth unexpectedly succeeded with an unreachable address"
fi
grep -qi "Could not connect\|unreachable\|Invalid address\|not available" /tmp/control_bt.log \
    || fail "control did not report a Bluetooth connection failure"
echo "ok: control fails cleanly over Bluetooth when the robot is unreachable"

echo "=== Step 5: calibrate with no paired device fails cleanly ==="
if "$CLI" calibrate --yl 10 --yr 0 --rl -5 --rr 2 >/tmp/calib_out.log 2>&1; then
    fail "calibrate with an empty session unexpectedly succeeded"
fi
grep -qi "No paired device" /tmp/calib_out.log || fail "calibrate did not report the missing device"
echo "ok: calibrate fails cleanly with no paired device"

echo "=== Step 6: calibrate over USB without a present port fails cleanly ==="
if "$CLI" calibrate --backend usb --address /dev/ttyUSB99 --yl 10 -t 1 >/tmp/calib_usb.log 2>&1; then
    fail "calibrate over USB unexpectedly succeeded with no port"
fi
grep -qi "No USB serial ports found\|Plug in the robot\|not available" /tmp/calib_usb.log \
    || fail "calibrate did not report the missing USB port"
echo "ok: calibrate fails cleanly over USB when no port is present"

echo "All clean-failure-path tests passed."
