#!/usr/bin/env bash
# Local test runner for ZowiDesktop.
#
# Builds the CLI (unless --no-build) and runs the automated suites that need no
# physical robot:
#   - white-box unit tests (src/core/tests) via CTest
#   - black-box CLI tests via CTest (cli_blackbox on Linux / cli_blackbox_win on Windows)
#
# On Linux this also runs the USB/Bluetooth "no-hardware-only" reachability
# checks through the canonical black-box orchestrator. Hardware-dependent tests
# are never run here; use the src/cli/tests/{bt,usb}/run_all.sh scripts for those.
#
# Usage:
#   ./run-tests.sh                 # build + ctest (Linux or Windows/MSYS)
#   ./run-tests.sh --verbose       # also show the black-box CLI detail
#   ./run-tests.sh --no-build      # run only, reusing an existing build
#   ./run-tests.sh -5              # build against Qt 5 (Linux only)
set -uo pipefail

cd "$(dirname "$0")"

VERBOSE=0
NO_BUILD=0
QT_FLAGS=()

for arg in "$@"; do
    case "$arg" in
        --verbose) VERBOSE=1 ;;
        --no-build) NO_BUILD=1 ;;
        -5) QT_FLAGS+=(-5) ;;
        -6) QT_FLAGS+=(-6) ;;
        -h|--help)
            echo "Usage: ./run-tests.sh [--verbose] [--no-build] [-5|-6]"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            echo "Usage: ./run-tests.sh [--verbose] [--no-build] [-5|-6]" >&2
            exit 2
            ;;
    esac
done

OS="$(uname -s)"
if [[ "$OS" == MINGW* || "$OS" == MSYS* || "$OS" == CYGWIN* ]]; then
    PLATFORM="windows"
else
    PLATFORM="linux"
fi
echo "== Platform: $PLATFORM =="

# ── Build ─────────────────────────────────────────────────────
if [ "$NO_BUILD" = "1" ]; then
    echo "== Skipping build (--no-build) =="
elif [ "$PLATFORM" = "windows" ]; then
    echo "== Building CLI (build.bat --cli) =="
    cmd //c "build.bat --cli" || { echo "FAIL: build failed" >&2; exit 1; }
else
    echo "== Building CLI (build.sh --cli) =="
    ./build.sh --cli "${QT_FLAGS[@]}" || { echo "FAIL: build failed" >&2; exit 1; }
fi

# ── White-box + black-box via CTest ───────────────────────────
echo ""
echo "== Running CTest (white-box + black-box) =="
if [ "$PLATFORM" = "windows" ]; then
    ctest --test-dir build -C Release
    RC=$?
else
    ctest --test-dir build
    RC=$?
fi
if [ "$RC" -ne 0 ]; then
    echo "FAIL: ctest failed" >&2
    exit 1
fi

# ── Optional: verbose black-box detail ────────────────────────
if [ "$VERBOSE" = "1" ]; then
    echo ""
    echo "== Black-box CLI detail =="
    if [ "$PLATFORM" = "windows" ]; then
        scripts/test/run-cli-blackbox-win.sh
    else
        scripts/test/run-cli-blackbox.sh
    fi
    if [ "$?" -ne 0 ]; then
        echo "FAIL: black-box CLI tests failed" >&2
        exit 1
    fi
fi

echo ""
echo "All local automated tests passed."
