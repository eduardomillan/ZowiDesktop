#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
QT_VERSION=6
TARGET="all"
RUN_DEMO=0

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Build Zowi Desktop (GUI, CLI, or both).

Options:
  -5            Build against Qt 5 (default: Qt 6)
  -6            Build against Qt 6 (default)

  --gui         Build GUI only (ZowiDesktop)
  --cli         Build CLI only (zowi_cli)
  --demo        Build CLI and run demo commands
  --all         Build everything (default)
  -h            Show this help message

Environment:
  QT_PATH       Path to Qt installation (e.g. ~/Qt/6.5.2/gcc_64)

Examples:
  ./build.sh                  # Build GUI + CLI with Qt 6
  ./build.sh --gui            # Build only the GUI
  ./build.sh --cli            # Build only the CLI
  ./build.sh --demo           # Build CLI and run demo commands
  ./build.sh -5 --cli         # Build CLI with Qt 5
  ./build.sh -6 --gui         # Build GUI with Qt 6 (explicit)
  QT_PATH=~/Qt/5.15.2/gcc_64 ./build.sh -5 --all

Targets:
  ZowiDesktop    Qt Quick GUI application  (build/ZowiDesktop)
  zowi_cli       Command-line tool          (build/src/cli/zowi_cli)
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        5) QT_VERSION=5; shift ;;
        6) QT_VERSION=6; shift ;;
        --gui) TARGET="gui"; shift ;;
        --cli) TARGET="cli"; shift ;;
        --demo) TARGET="cli"; RUN_DEMO=1; shift ;;
        --all) TARGET="all"; shift ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1"; usage ;;
    esac
done

CMAKE_EXTRA_ARGS=""
if [ -n "${QT_PATH:-}" ]; then
    CMAKE_EXTRA_ARGS="-DCMAKE_PREFIX_PATH=$QT_PATH"
fi

BUILD_GUI=OFF
BUILD_CLI=OFF
case "$TARGET" in
    all) BUILD_GUI=ON; BUILD_CLI=ON ;;
    gui) BUILD_GUI=ON ;;
    cli) BUILD_CLI=ON ;;
esac

echo "=== Building Zowi Desktop (Qt $QT_VERSION) ==="
echo "    GUI: $BUILD_GUI | CLI: $BUILD_CLI"

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DZOWI_QT_VERSION="$QT_VERSION" \
    -DZOWI_BUILD_GUI="$BUILD_GUI" \
    -DZOWI_BUILD_CLI="$BUILD_CLI" \
    $CMAKE_EXTRA_ARGS

if [ "$BUILD_GUI" = "ON" ]; then
    cmake --build "$BUILD_DIR" --target ZowiDesktop
fi

if [ "$BUILD_CLI" = "ON" ]; then
    cmake --build "$BUILD_DIR" --target zowi_cli
fi

echo ""
echo "Build complete (Qt $QT_VERSION)."

if [ "$BUILD_GUI" = "ON" ]; then
    echo "  GUI: $BUILD_DIR/ZowiDesktop"
fi

if [ "$BUILD_CLI" = "ON" ]; then
    echo "  CLI: $BUILD_DIR/src/cli/zowi_cli"
fi

# ── Demo mode ──────────────────────────────────────────────
if [ "$RUN_DEMO" = "1" ]; then
    CLI="$BUILD_DIR/src/cli/zowi_cli"

    echo ""
    echo "=== session ==="
    echo "$ $CLI session list"
    $CLI session list
    echo ""

    echo "$ $CLI session get wizardDismissed"
    $CLI session get wizardDismissed
    echo ""

    echo "=== config ==="
    echo "$ $CLI config list"
    $CLI config list
    echo ""

    echo "$ $CLI config get know_more"
    $CLI config get know_more
    echo ""

    echo "=== translate ==="
    echo "$ $CLI translate -s 'Hola mundo'"
    $CLI translate -s "Hola mundo"
    echo ""

    echo "$ $CLI translate -l en_US -s 'Hola mundo'"
    $CLI translate -l en_US -s "Hola mundo"
    echo ""

    echo "=== scan ==="
    echo "$ $CLI scan"
    $CLI scan
    echo ""

    echo "=== help ==="
    $CLI --help
fi
