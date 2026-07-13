#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
QT_VERSION=6

usage() {
    cat <<EOF
Usage: $(basename "$0") <ScreenName> [preview options]

Builds and launches the isolated QML preview for a screen in src/views/screens/.

Examples:
  ./src/views/tests/preview-screen.sh SplashScreen
  ./src/views/tests/preview-screen.sh HomeScreen --connected
  ./src/views/tests/preview-screen.sh ScanScreen --locale es_ES

Qt selection:
  -5    Build against Qt 5
  -6    Build against Qt 6 (default)

Environment:
  QT_PATH   Path to Qt installation (e.g. ~/Qt/6.5.2/gcc_64)

Preview options are forwarded to zowi_screen_preview.
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -5) QT_VERSION=5; shift ;;
        -6) QT_VERSION=6; shift ;;
        -h|--help) usage ;;
        *) break ;;
    esac
done

if [[ $# -lt 1 ]]; then
    usage
fi

SCREEN_NAME="$1"
shift

SCREEN_PATH="$REPO_ROOT/src/views/screens/${SCREEN_NAME}.qml"
if [[ ! -f "$SCREEN_PATH" ]]; then
    echo "Screen not found: $SCREEN_PATH" >&2
    exit 1
fi

CMAKE_EXTRA_ARGS=()
if [[ -n "${QT_PATH:-}" ]]; then
    CMAKE_EXTRA_ARGS+=("-DCMAKE_PREFIX_PATH=$QT_PATH")
fi

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
    -DZOWI_QT_VERSION="$QT_VERSION" \
    -DZOWI_BUILD_GUI=ON \
    -DZOWI_BUILD_CLI=OFF \
    "${CMAKE_EXTRA_ARGS[@]}"

cmake --build "$BUILD_DIR" --target zowi_screen_preview

cd "$REPO_ROOT"
exec "$BUILD_DIR/src/gui/zowi_screen_preview" "$SCREEN_PATH" "$@"
