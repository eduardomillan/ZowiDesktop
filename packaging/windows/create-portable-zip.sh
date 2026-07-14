#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-windows"
DIST_DIR="$BUILD_DIR/dist"
APP_NAME="ZowiDesktop"
QT_HOST_PATH="${QT_HOST_PATH:-$HOME/Qt/6.5.2/gcc_64}"
QT_TARGET_PATH="${QT_TARGET_PATH:-$HOME/Qt/6.5.2/6.5.2/mingw_64}"

echo "=== Step 1: Check Qt for MinGW ==="
if [ ! -f "$QT_TARGET_PATH/bin/qmake6.exe" ] && [ ! -f "$QT_TARGET_PATH/bin/qmake.exe" ]; then
    echo "Qt6 for MinGW not found at $QT_TARGET_PATH"
    echo ""
    echo "Install with aqtinstall:"
    echo "  python3 -m aqt install-qt windows desktop 6.5.2 win64_mingw -m qtconnectivity"
    echo ""
    echo "Then set QT_TARGET_PATH, e.g.:"
    echo "  export QT_TARGET_PATH=\$HOME/Qt/6.5.2/6.5.2/mingw_64"
    echo "  $0"
    exit 1
fi

echo "=== Step 2: Configure CMake ==="
rm -rf "$BUILD_DIR"
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/packaging/windows/mingw-toolchain.cmake" \
    -DCMAKE_PREFIX_PATH="$QT_TARGET_PATH" \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_HOST_PATH="$QT_HOST_PATH" \
    -DZOWI_BUILD_CLI=OFF \
    -DBUILD_TESTS=OFF

echo ""
echo "=== Step 3: Build ==="
cmake --build "$BUILD_DIR"

echo ""
echo "=== Step 4: Collect Qt6 DLLs and plugins ==="
mkdir -p "$DIST_DIR/platforms"
mkdir -p "$DIST_DIR/imageformats"
mkdir -p "$DIST_DIR/styles"
mkdir -p "$DIST_DIR/Qt/qml"

# Core Qt6 DLLs
for dll in Qt6Core Qt6Gui Qt6Widgets Qt6Quick Qt6Qml Qt6QmlModels \
           Qt6QmlWorkerScript Qt6Network Qt6DBus Qt6Bluetooth Qt6Svg \
           Qt6OpenGL Qt6QuickControls2 Qt6QuickControls2Impl Qt6QuickTemplates2; do
    cp "$QT_TARGET_PATH/bin/$dll.dll" "$DIST_DIR/"
done

# MinGW runtime
cp "$QT_TARGET_PATH/bin/libwinpthread-1.dll" "$DIST_DIR/"
cp "$QT_TARGET_PATH/bin/libgcc_s_seh-1.dll" "$DIST_DIR/" 2>/dev/null || \
cp /usr/lib/gcc/x86_64-w64-mingw32/10-win32/libgcc_s_seh-1.dll "$DIST_DIR/"
cp "$QT_TARGET_PATH/bin/libstdc++-6.dll" "$DIST_DIR/" 2>/dev/null || \
cp /usr/lib/gcc/x86_64-w64-mingw32/10-win32/libstdc++-6.dll "$DIST_DIR/"

# Platform plugins
cp "$QT_TARGET_PATH/plugins/platforms/qwindows.dll" "$DIST_DIR/platforms/"

# Image format plugins
cp "$QT_TARGET_PATH/plugins/imageformats/"*.dll "$DIST_DIR/imageformats/" 2>/dev/null || true

# Style plugins
cp "$QT_TARGET_PATH/plugins/styles/"*.dll "$DIST_DIR/styles/" 2>/dev/null || true

# QML modules (copy only what's needed for QtQuick.Controls)
for qml_dir in QtQuick QtQml QtCore; do
    if [ -d "$QT_TARGET_PATH/qml/$qml_dir" ]; then
        cp -r "$QT_TARGET_PATH/qml/$qml_dir" "$DIST_DIR/Qt/qml/"
    fi
done

echo ""
echo "=== Step 5: Copy executable ==="
EXE_PATH=$(find "$BUILD_DIR" -maxdepth 4 -name "${APP_NAME}.exe" | head -1)
if [ -z "$EXE_PATH" ]; then
    echo "ERROR: $APP_NAME.exe not found in build tree"
    exit 1
fi
echo "  (from $EXE_PATH)"
cp "$EXE_PATH" "$DIST_DIR/"

echo "=== Step 5b: Copy qt.conf (QML/plugin paths) ==="
cp "$PROJECT_ROOT/packaging/windows/qt.conf" "$DIST_DIR/"

echo ""
echo "=== Step 6: Create portable zip ==="
TIMESTAMP=$(date +%Y%m%d)
cd "$DIST_DIR"
rm -f "${APP_NAME}-windows-x86_64-build-${TIMESTAMP}.zip"
zip -r "${APP_NAME}-windows-x86_64-build-${TIMESTAMP}.zip" .
cd "$PROJECT_ROOT"

echo ""
echo "=== Done ==="
ls -lh "$DIST_DIR/${APP_NAME}-windows-x86_64-build-${TIMESTAMP}.zip"
