#!/usr/bin/env bash
# Runs the USB CLI tests in order.
#
# By default only the hardware-free smoke tests run (ports enumeration and
# option parsing). Set ZOWI_USB_FLASH=1 to also run the flashing tests, which
# require a robot connected over USB. Provide ZOWI_USB_TTY / ZOWI_USB_BAUD to
# target a specific port and speed.
#
# Environment:
#   ZOWI_CLI        Path to the zowi_cli binary (default: build/src/cli/zowi_cli)
#   ZOWI_USB_FLASH  Set to 1 to run the restore/alarm flashing tests (needs a robot)
#   ZOWI_USB_TTY    USB serial port to use for flashing (e.g. /dev/ttyUSB0)
#   ZOWI_USB_BAUD   Baud rate for flashing (default in each test: 115200)
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CLI="${ZOWI_CLI:-build/src/cli/zowi_cli}"
FLASH="${ZOWI_USB_FLASH:-0}"

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

# 1. Hardware-free smoke tests (always run).
run "usb/test_usb_options.sh" "$SCRIPT_DIR/test_usb_options.sh"
run "usb/test_ports.sh"       "$SCRIPT_DIR/test_ports.sh"

# 2. Flashing tests (opt-in, require a robot connected over USB).
if [ "$FLASH" = "1" ]; then
    run "usb/test_restore_factory_firmware.sh" "$SCRIPT_DIR/test_restore_factory_firmware.sh"
    run "usb/test_install_alarm.sh"            "$SCRIPT_DIR/test_install_alarm.sh"
else
    echo ""
    echo "SKIP: flashing tests (set ZOWI_USB_FLASH=1 and connect a robot to run them)"
fi

echo ""
echo "========================================================"
echo "USB test summary: $pass passed, $fail failed"
if [ "$fail" -ne 0 ]; then
    printf '  - %s\n' "${failed_tests[@]}"
    exit 1
fi
echo "All USB tests passed."
