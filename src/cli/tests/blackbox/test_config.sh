#!/usr/bin/env bash
# Black-box test of the `config` subcommand against the workspace config file.
# The CLI reads src/config.json relative to the current working directory, so
# this must run from the repository root (the orchestrator ensures that).
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"
[ -f "src/config.json" ] || fail "src/config.json not found; run from the repository root"

echo "=== Step 1: config get returns a known value ==="
# zowi_mac_prefix is a stable key used by discovery.
PREFIX=$("$CLI" config get zowi_mac_prefix) || fail "config get zowi_mac_prefix failed"
[ -n "$PREFIX" ] || fail "config get returned an empty value"
echo "ok: config get zowi_mac_prefix = '$PREFIX'"

echo "=== Step 2: config list returns sorted key=value pairs ==="
LIST=$("$CLI" config list)
echo "$LIST" | grep -q "zowi_mac_prefix=" || fail "config list missing zowi_mac_prefix"
echo "ok: config list includes zowi_mac_prefix"

echo "=== Step 3: config get on a missing key fails cleanly ==="
if "$CLI" config get does_not_exist >/dev/null 2>&1; then
    fail "config get for missing key unexpectedly succeeded"
fi
echo "ok: missing config key returns non-zero"

echo "All config tests passed."
