#!/usr/bin/env bash
# Black-box test orchestrator for the zowi_cli command-line interface.
#
# Runs every CLI case that requires NO physical robot, so it is safe to execute
# locally and in CI. Hardware-dependent tests (src/cli/tests/{bt,usb}) are out
# of scope here and are reported as SKIP.
#
# Usage:
#   scripts/test/run-cli-blackbox.sh
#
# Environment:
#   ZOWI_CLI   Path to the zowi_cli binary (default: build/src/cli/zowi_cli)
#   NO_BUILD   Set to 1 to skip auto-building the CLI (assume it exists)
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLI="${ZOWI_CLI:-$REPO_ROOT/build/src/cli/zowi_cli}"

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

# The CLI must be run from the repository root: `translate` and `config` read
# i18n/ and src/config.json relative to the current working directory.
cd "$REPO_ROOT"

# Build the CLI on first use if it is missing, unless asked not to.
if ! command -v "$CLI" >/dev/null 2>&1; then
    if [ "${NO_BUILD:-0}" = "1" ]; then
        echo "FAIL: zowi_cli not found at $CLI and NO_BUILD=1" >&2
        exit 1
    fi
    echo "zowi_cli not found; building it..."
    cmake -S . -B build -DZOWI_BUILD_GUI=OFF -DZOWI_BUILD_CLI=ON >/dev/null 2>&1 \
        || { echo "FAIL: cmake configure failed" >&2; exit 1; }
    cmake --build build --target zowi_cli >/dev/null 2>&1 \
        || { echo "FAIL: cmake build failed" >&2; exit 1; }
fi

export ZOWI_CLI="$CLI"

# Isolate all session state in a throwaway dir so the tests never touch the
# user's real ZowiDesktop session data (SessionStore honors XDG_CONFIG_HOME).
TMP_SESSION="$(mktemp -d)"
trap 'rm -rf "$TMP_SESSION"' EXIT
export XDG_CONFIG_HOME="$TMP_SESSION"

BB="$REPO_ROOT/src/cli/tests/blackbox"

# 1. Deterministic, no-hardware black-box cases.
run "blackbox/test_help.sh"           "$BB/test_help.sh"
run "blackbox/test_session.sh"        "$BB/test_session.sh"
run "blackbox/test_config.sh"         "$BB/test_config.sh"
run "blackbox/test_translate.sh"      "$BB/test_translate.sh"
run "blackbox/test_failure_paths.sh"  "$BB/test_failure_paths.sh"

# 2. Existing CLI tests that also run without a robot. They are invoked from the
#    repo root and reuse the same ZOWI_CLI/XDG isolation.
T="$REPO_ROOT/src/cli/tests"
run "usb/test_usb_options.sh" "$T/usb/test_usb_options.sh"
run "usb/test_ports.sh"       "$T/usb/test_ports.sh"
run "usb/test_control.sh"     "$T/usb/test_control.sh"
run "bt/test_control.sh"      "$T/bt/test_control.sh"

# 3. Hardware-dependent suites are out of scope; report them as skippable.
REQUIRE_HW=(
    "bt/test_connect_rename.sh  (ZOWI_BT_FULL=1  + robot in range)"
    "bt/test_disconnect.sh      (robot in range)"
    "bt/test_restore_factory_firmware.sh (ZOWI_BT_FULL=1)"
    "bt/test_install_alarm.sh          (ZOWI_BT_FULL=1)"
    "bt/test_install_adivinawi.sh      (ZOWI_BT_FULL=1)"
    "usb/test_connect_rename.sh (ZOWI_USB_CONNECT=1 + robot over USB)"
    "usb/test_disconnect.sh     (ZOWI_USB_CONNECT=1)"
    "usb/test_restore_factory_firmware.sh (ZOWI_USB_FLASH=1)"
    "usb/test_install_alarm.sh          (ZOWI_USB_FLASH=1)"
    "usb/test_install_adivinawi.sh      (ZOWI_USB_FLASH=1)"
)
echo ""
echo "SKIP (require physical hardware; run locally via their run_all.sh):"
printf '  - %s\n' "${REQUIRE_HW[@]}"

echo ""
echo "========================================================"
echo "CLI black-box summary: $pass passed, $fail failed"
if [ "$fail" -ne 0 ]; then
    printf '  - %s\n' "${failed_tests[@]}"
    exit 1
fi
echo "All CLI black-box tests passed."
