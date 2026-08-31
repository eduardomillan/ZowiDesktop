#!/usr/bin/env bash
# Black-box smoke test: every CLI subcommand must be documented via --help and
# must list its expected options. No robot or hardware required.
set -euo pipefail

CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"

fail() { echo "FAIL: $1" >&2; exit 1; }

command -v "$CLI" >/dev/null 2>&1 || fail "zowi_cli not found at $CLI (build it first: ./build.sh --cli)"

# Every subcommand must appear in the top-level help.
for sub in session translate config ports scan connect rename restore alarm adivinawi disconnect status control; do
    "$CLI" --help 2>&1 | grep -qw -- "$sub" \
        || fail "top-level --help missing '$sub' subcommand"
done
echo "ok: all 13 subcommands are listed in top-level --help"

# Each subcommand must have a --help routing and expose its key options.
"$CLI" session   --help 2>&1 | grep -qw "get"  || fail "session --help missing 'get'"
"$CLI" session   --help 2>&1 | grep -qw "set"  || fail "session --help missing 'set'"
"$CLI" session   --help 2>&1 | grep -qw "list" || fail "session --help missing 'list'"
"$CLI" session   --help 2>&1 | grep -qw "clear" || fail "session --help missing 'clear'"

"$CLI" translate --help 2>&1 | grep -q -- "--locale" || fail "translate --help missing --locale"
"$CLI" translate --help 2>&1 | grep -q -- "--context" || fail "translate --help missing --context"
"$CLI" translate --help 2>&1 | grep -q -- "--source"  || fail "translate --help missing --source"

"$CLI" config    --help 2>&1 | grep -qw "get"  || fail "config --help missing 'get'"
"$CLI" config    --help 2>&1 | grep -qw "list" || fail "config --help missing 'list'"

"$CLI" scan      --help 2>&1 | grep -q -- "--timeout" || fail "scan --help missing --timeout"

"$CLI" connect   --help 2>&1 | grep -q -- "--backend" || fail "connect --help missing --backend"
"$CLI" connect   --help 2>&1 | grep -q -- "--tty"     || fail "connect --help missing --tty"
"$CLI" connect   --help 2>&1 | grep -q -- "--baud"    || fail "connect --help missing --baud"

"$CLI" rename    --help 2>&1 | grep -q -- "--backend" || fail "rename --help missing --backend"
"$CLI" status    --help 2>&1 | grep -q -- "--backend" || fail "status --help missing --backend"

for subcmd in restore alarm adivinawi; do
    "$CLI" "$subcmd" --help 2>&1 | grep -q -- "--firmware"  || fail "$subcmd --help missing --firmware"
    "$CLI" "$subcmd" --help 2>&1 | grep -q -- "--protocol"  || fail "$subcmd --help missing --protocol"
    "$CLI" "$subcmd" --help 2>&1 | grep -q -- "--backend"   || fail "$subcmd --help missing --backend"
    "$CLI" "$subcmd" --help 2>&1 | grep -q -- "--force-low-battery" || fail "$subcmd --help missing --force-low-battery"
done

"$CLI" control   --help 2>&1 | grep -q -- "--speed"  || fail "control --help missing --speed"
"$CLI" control   --help 2>&1 | grep -q -- "--backend" || fail "control --help missing --backend"

echo "All --help smoke tests passed."
