#!/usr/bin/env bash
# Windows black-box test orchestrator for the zowi_cli command-line interface.
#
# Runs the CLI cases that are backend-agnostic (stable across Windows and
# Linux) and require NO physical robot. It is designed to run in GitHub Actions
# on windows-latest under Git Bash / MSYS, and to be driven by CTest.
#
# It intentionally does NOT run the USB/Bluetooth reachability cases from
# scripts/test/run-cli-blackbox.sh: on Windows the backend is bt_native (WinRT)
# and WinSerial COM ports, whose output differs from Linux/BlueZ and cannot be
# reliably asserted in CI.
#
# Usage:
#   scripts/test/run-cli-blackbox-win.sh
#
# Environment:
#   ZOWI_CLI   Path to zowi_cli (default: build/src/cli/Release/zowi_cli)
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLI="${ZOWI_CLI:-$REPO_ROOT/build/src/cli/Release/zowi_cli}"

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

cd "$REPO_ROOT"

# On Windows the binary is zowi_cli.exe; the shell adds the .exe extension when
# invoking, but the existence check needs the real file name.
if [ ! -f "$CLI" ]; then
    if [ -f "$CLI.exe" ]; then
        CLI="$CLI.exe"
    fi
fi
if [ ! -f "$CLI" ]; then
    echo "FAIL: zowi_cli not found at $CLI (build the CLI target in Release first)" >&2
    exit 1
fi
export ZOWI_CLI="$CLI"

# Isolate session state in a throwaway dir. On Windows SessionStore honors the
# APPDATA env var (session_store.cpp), so point it at a temp dir inside the repo
# and convert to a native Windows path for the CLI executable.
TMP_SESSION="$REPO_ROOT/build/test-apdata-win"
rm -rf "$TMP_SESSION"
mkdir -p "$TMP_SESSION"
trap 'rm -rf "$TMP_SESSION"' EXIT
if command -v cygpath >/dev/null 2>&1; then
    export APPDATA="$(cygpath -w "$TMP_SESSION")"
else
    export APPDATA="$TMP_SESSION"
fi

BB="$REPO_ROOT/src/cli/tests/blackbox"

run "blackbox/test_help.sh"             "$BB/test_help.sh"
run "blackbox/test_session.sh"          "$BB/test_session.sh"
run "blackbox/test_config.sh"           "$BB/test_config.sh"
run "blackbox/test_translate.sh"        "$BB/test_translate.sh"
run "blackbox/test_failure_paths_win.sh" "$BB/test_failure_paths_win.sh"

echo ""
echo "========================================================"
echo "Windows CLI black-box summary: $pass passed, $fail failed"
if [ "$fail" -ne 0 ]; then
    printf '  - %s\n' "${failed_tests[@]}"
    exit 1
fi
echo "All Windows CLI black-box tests passed."
