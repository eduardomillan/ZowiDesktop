#!/usr/bin/env bash
# Black-box test of the `session` subcommand: set/get/list/clear with
# string, bool and int values, plus the clean error path for a missing key.
#
# The store lives under $XDG_CONFIG_HOME (see SessionStore::resolveConfigPath),
# which the orchestrator points at a fresh temp dir so this never touches the
# user's real session data.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"

echo "=== Step 1: clear ensures a clean slate ==="
"$CLI" session clear >/dev/null 2>&1
OUT=$("$CLI" session list)
[ -z "$OUT" ] || fail "expected empty session after clear, got: $OUT"
echo "ok: session is empty after clear"

echo "=== Step 2: set string/bool/int then get them back ==="
"$CLI" session set activeZowiName "R2D2"      >/dev/null || fail "session set string failed"
"$CLI" session set wizardDismissed true        >/dev/null || fail "session set bool failed"
"$CLI" session set activeZowiBattery 87        >/dev/null || fail "session set int failed"

[ "$("$CLI" session get activeZowiName)" = "R2D2" ]   || fail "get string mismatch"
[ "$("$CLI" session get wizardDismissed)" = "true" ]  || fail "get bool mismatch"
[ "$("$CLI" session get activeZowiBattery)" = "87" ]  || fail "get int mismatch"
echo "ok: string/bool/int round-trip works"

echo "=== Step 3: list shows all keys ==="
LIST=$("$CLI" session list)
echo "$LIST" | grep -q "activeZowiName=R2D2"    || fail "list missing activeZowiName"
echo "$LIST" | grep -q "wizardDismissed=true"   || fail "list missing wizardDismissed"
echo "$LIST" | grep -q "activeZowiBattery=87"   || fail "list missing activeZowiBattery"
echo "ok: list shows all session keys"

echo "=== Step 4: get on a missing key fails cleanly ==="
if "$CLI" session get does_not_exist >/dev/null 2>&1; then
    fail "session get for missing key unexpectedly succeeded"
fi
echo "ok: missing key returns non-zero"

echo "All session tests passed."
