#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APPDIR="$PROJECT_ROOT/build/AppDir"
TOOLS_DIR="$PROJECT_ROOT/build/.tools"

echo "=== Step 1: Build project ==="
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD_DIR"

echo ""
echo "=== Step 2: Install to AppDir ==="
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

echo ""
echo "=== Step 3: Prepare AppDir resources ==="
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/metainfo"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

cp "$PROJECT_ROOT/packaging/linux/zowi-desktop.desktop" "$APPDIR/usr/share/applications/"
cp "$PROJECT_ROOT/packaging/linux/zowi-desktop.appdata.xml" "$APPDIR/usr/share/metainfo/"
cp "$PROJECT_ROOT/images/app_icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/zowi-desktop.png"
cp "$PROJECT_ROOT/images/app_icon.png" "$APPDIR/zowi-desktop.png"

echo ""
echo "=== Step 4: Download linuxdeploy ==="
mkdir -p "$TOOLS_DIR"
if [ ! -f "$TOOLS_DIR/linuxdeploy-x86_64.AppImage" ]; then
    wget -q -O "$TOOLS_DIR/linuxdeploy-x86_64.AppImage" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
fi
if [ ! -f "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    wget -q -O "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
fi

echo ""
echo "=== Step 5: Run linuxdeploy ==="
export LDAI_OUTPUT="ZowiDesktop-x86_64.AppImage"
export QML_SOURCES_PATHS="$PROJECT_ROOT/qml"
export EXTRA_QT_MODULES="quick;xml"

# Remove AppStream metadata to avoid validation errors during AppImage creation
rm -f "$APPDIR/usr/share/metainfo/zowi-desktop.appdata.xml"

"$TOOLS_DIR/linuxdeploy-x86_64.AppImage" \
    --appdir "$APPDIR" \
    --plugin qt \
    --desktop-file "$APPDIR/usr/share/applications/zowi-desktop.desktop" \
    --icon-file "$APPDIR/zowi-desktop.png" \
    --output appimage

echo ""
echo "=== Moving AppImage to build directory ==="
mv "$PROJECT_ROOT/ZowiDesktop-x86_64.AppImage" "$PROJECT_ROOT/build/" 2>/dev/null || true

echo ""
echo "=== Done ==="
ls -lh "$BUILD_DIR/ZowiDesktop-x86_64.AppImage"
