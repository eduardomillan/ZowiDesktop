#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-windows"
DIST_DIR="$BUILD_DIR/dist"
APP_NAME="ZowiDesktop"
QT_MINGW_PATH="${QT_MINGW_PATH:-$HOME/Qt/5.15.2/mingw81_64}"

echo "=== Step 1: Check Qt for MinGW ==="
if [ ! -f "$QT_MINGW_PATH/bin/qmake.exe" ]; then
    echo "Qt for MinGW not found at $QT_MINGW_PATH"
    echo ""
    echo "Download and install Qt 5.15 for MinGW from:"
    echo "  https://www.qt.io/download-open-source"
    echo ""
    echo "Then set QT_MINGW_PATH, e.g.:"
    echo "  export QT_MINGW_PATH=\$HOME/Qt/5.15.2/mingw81_64"
    echo "  $0"
    exit 1
fi

export PATH="$QT_MINGW_PATH/bin:$PATH"

echo "=== Step 2: Configure CMake ==="
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_FIND_ROOT_PATH="$QT_MINGW_PATH" \
    -DCMAKE_PREFIX_PATH="$QT_MINGW_PATH" \
    -DCMAKE_BUILD_TYPE=Release

echo ""
echo "=== Step 3: Build ==="
cmake --build "$BUILD_DIR"

echo ""
echo "=== Step 4: Collect DLLs with windeployqt ==="
mkdir -p "$DIST_DIR"
"$QT_MINGW_PATH/bin/windeployqt.exe" --dir "$DIST_DIR" "$BUILD_DIR/${APP_NAME}.exe"

echo ""
echo "=== Step 5: Copy executable ==="
cp "$BUILD_DIR/${APP_NAME}.exe" "$DIST_DIR/"

echo ""
echo "=== Step 6: Create portable zip ==="
cd "$DIST_DIR"
zip -r "${APP_NAME}-windows-x86_64.zip" .
cd "$PROJECT_ROOT"

echo ""
echo "=== Done ==="
ls -lh "$DIST_DIR/${APP_NAME}-windows-x86_64.zip"
